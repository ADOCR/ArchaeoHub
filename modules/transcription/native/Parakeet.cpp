// Decodificación NeMo TDT adaptada de onnx-asr (MIT); consulta THIRD_PARTY.md.
#include "Parakeet.h"
#include <onnxruntime_cxx_api.h>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <algorithm>
#include <stdexcept>
#include <array>

namespace {
Ort::Value floats(float *p,size_t n,std::initializer_list<int64_t> shape) {
    std::vector<int64_t> s(shape); auto m=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
    return Ort::Value::CreateTensor<float>(m,p,n,s.data(),s.size());
}
template<class T> Ort::Value integer(T *p,std::initializer_list<int64_t> shape) {
    std::vector<int64_t> s(shape); auto m=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
    return Ort::Value::CreateTensor<T>(m,p,1,s.data(),s.size());
}
Ort::Session session(Ort::Env &env,Ort::SessionOptions &opt,const QString &path) {
#ifdef Q_OS_WIN
    return Ort::Session(env,reinterpret_cast<const wchar_t*>(path.utf16()),opt);
#else
    return Ort::Session(env,path.toUtf8().constData(),opt);
#endif
}
}
struct Parakeet::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_ERROR,"Gamboa"}; Ort::SessionOptions options;
    std::unique_ptr<Ort::Session> prep,encoder,decoder,vad;
    QStringList vocab; int blank=0;
    explicit Impl(const QString &folder) {
        options.SetIntraOpNumThreads(6); options.SetInterOpNumThreads(1);
        auto load=[&](const QString &name){ return std::make_unique<Ort::Session>(session(env,options,QDir(folder).filePath(name))); };
        prep=load("nemo128.onnx"); encoder=load("encoder-model.int8.onnx");
        decoder=load("decoder_joint-model.int8.onnx"); vad=load("silero_vad.onnx");
        QFile f(QDir(folder).filePath("vocab.txt")); if(!f.open(QIODevice::ReadOnly)) throw std::runtime_error("Falta el vocabulario de Parakeet.");
        while(!f.atEnd()) {
            QString line=QString::fromUtf8(f.readLine()).trimmed(); int split=line.lastIndexOf(' ');
            if(split<0) continue; int id=line.mid(split+1).toInt(); while(vocab.size()<=id) vocab<<QString();
            vocab[id]=line.left(split).replace(QChar(0x2581),' '); if(vocab[id]=="<blk>") blank=id;
        }
    }
    std::vector<std::pair<size_t,size_t>> speech(const std::vector<float> &audio,std::atomic_bool &cancel) {
        std::vector<std::pair<size_t,size_t>> raw; std::vector<float> state(256,0),frame(576,0);
        bool active=false; size_t start=0,lastVoice=0;
        const char *ins[]={"input","state","sr"},*outs[]={"output","stateN"}; int64_t sr=16000;
        for(size_t pos=0;pos<audio.size();pos+=512) {
            if(cancel) throw std::runtime_error("Proceso cancelado.");
            std::fill(frame.begin(),frame.end(),0);
            for(int i=0;i<576;++i) { auto at=static_cast<qint64>(pos)+i-64; if(at>=0 && at<static_cast<qint64>(audio.size())) frame[i]=audio[at]; }
            std::array<Ort::Value,3> args={floats(frame.data(),576,{1,576}),floats(state.data(),256,{2,1,128}),integer(&sr,{})};
            auto out=vad->Run(Ort::RunOptions{nullptr},ins,args.data(),3,outs,2);
            float probability=out[0].GetTensorData<float>()[0];
            std::copy_n(out[1].GetTensorData<float>(),256,state.data());
            if(probability>=0.5f) { if(!active){active=true;start=pos;} lastVoice=pos+512; }
            if(active && ((probability<0.35f && pos>lastVoice+5600) || pos-start>=320000)) {
                size_t end=pos-start>=320000 ? pos : lastVoice;
                raw.emplace_back(start>2400?start-2400:0,std::min(end+2400,audio.size())); active=false;
            }
        }
        if(active) raw.emplace_back(start>2400?start-2400:0,audio.size());
        for(size_t i=1;i<raw.size();++i) if(raw[i].first<raw[i-1].second) {
            size_t mid=(raw[i].first+raw[i-1].second)/2; raw[i-1].second=mid; raw[i].first=mid;
        }
        return raw;
    }
    QString recognize(const float *wave,size_t length,std::atomic_bool &cancel) {
        int64_t len=length;
        std::array<Ort::Value,2> args={floats(const_cast<float*>(wave),length,{1,len}),integer(&len,{1})};
        const char *pins[]={"waveforms","waveforms_lens"},*pouts[]={"features","features_lens"};
        auto features=prep->Run(Ort::RunOptions{nullptr},pins,args.data(),2,pouts,2);
        const char *eins[]={"audio_signal","length"},*eouts[]={"outputs","encoded_lengths"};
        auto encoded=encoder->Run(Ort::RunOptions{nullptr},eins,features.data(),2,eouts,2);
        auto shape=encoded[0].GetTensorTypeAndShapeInfo().GetShape();
        int64_t frames=encoded[1].GetTensorData<int64_t>()[0],channels=shape[1],stride=shape[2];
        const float *enc=encoded[0].GetTensorData<float>();
        std::vector<float> state1(1280,0),state2(1280,0),frame(channels);
        int32_t previous=blank,one=1; int emitted=0; QString text;
        const char *dins[]={"encoder_outputs","targets","target_length","input_states_1","input_states_2"};
        const char *douts[]={"outputs","output_states_1","output_states_2"};
        for(int64_t t=0;t<frames;) {
            if(cancel) throw std::runtime_error("Proceso cancelado.");
            for(int64_t c=0;c<channels;++c) frame[c]=enc[c*stride+t];
            std::array<Ort::Value,5> input={floats(frame.data(),frame.size(),{1,channels,1}),integer(&previous,{1,1}),integer(&one,{1}),
                floats(state1.data(),1280,{2,1,640}),floats(state2.data(),1280,{2,1,640})};
            auto decoded=decoder->Run(Ort::RunOptions{nullptr},dins,input.data(),5,douts,3);
            const float *logits=decoded[0].GetTensorData<float>();
            int n=vocab.size(); int token=int(std::max_element(logits,logits+n)-logits);
            size_t count=decoded[0].GetTensorTypeAndShapeInfo().GetElementCount();
            int step=int(std::max_element(logits+n,logits+count)-(logits+n));
            if(token!=blank) { previous=token; text+=vocab.value(token); ++emitted;
                std::copy_n(decoded[1].GetTensorData<float>(),1280,state1.data()); std::copy_n(decoded[2].GetTensorData<float>(),1280,state2.data()); }
            if(step>0){t+=step;emitted=0;} else if(token==blank || emitted>=10){++t;emitted=0;}
        }
        text.remove(QRegularExpression("<\\|[^>]+\\|>"));
        text.replace(QRegularExpression("\\s+([,.;:!?])"),"\\1");
        return text.simplified();
    }
};
Parakeet::Parakeet(const QString &folder):p(std::make_unique<Impl>(folder)){}
Parakeet::~Parakeet()=default;
QVariantList Parakeet::transcribe(const std::vector<float> &audio,std::atomic_bool &cancel,const std::function<void(int,const QVariantList&)> &update) {
    auto spans=p->speech(audio,cancel); QVariantList result;
    for(const auto &span:spans) {
        auto text=p->recognize(audio.data()+span.first,span.second-span.first,cancel);
        if(!text.isEmpty()) result<<QVariantMap{{"start",qint64(span.first*1000/16000)},{"end",qint64(span.second*1000/16000)},{"text",text}};
        update(int(span.second*100/audio.size()),result);
    }
    return result;
}
