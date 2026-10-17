#include "UiChaos.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QTimer>
#include <memory>

namespace workbench {
void startUiChaos(QQuickWindow *window,const QString &directory,int iterations,quint32 seed,
                  ChaosServices s,std::function<void(QJsonObject)> completed){
    // Esta prueba rechaza carpetas existentes y crea su propio material sintético.
    if(QFileInfo::exists(directory)||!QDir().mkpath(directory)){completed({{"ok",false},{"error","QA directory must be new"}});return;}
    const QDir root(directory);root.mkpath("photos");root.mkpath("catalog");root.mkpath("exports");
    auto write=[&](const QString &name,const QByteArray &bytes){QFile file(root.filePath(name));return file.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&file.write(bytes)==bytes.size();};
    if(!write("Note.md","# Synthetic archaeological field note\nPozo 7: 14 ceramic fragments.\n")||!write("Corrupt.pdf","%PDF-1.7\nnot a valid PDF\n")||!write("Corrupt.wav","RIFFnot audio")){completed({{"ok",false},{"error","QA fixture creation failed"}});return;}
    for(int i=0;i<4;++i){QImage image(800,600,QImage::Format_RGB32);image.fill(QColor(30+i*12,90,100));if(!image.save(root.filePath(QString("photos/Photo-%1.jpg").arg(i)))){completed({{"ok",false},{"error","QA image creation failed"}});return;}}
    QHash<QString,QByteArray> originals;
    const auto names=QDir(root.filePath("photos")).entryList(QDir::Files);
    for(const auto &name:names){const auto path=root.filePath("photos/"+name);QFile file(path);file.open(QIODevice::ReadOnly);originals[path]=QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);}
    for(const auto &name:{"Note.md","Corrupt.pdf","Corrupt.wav"}){const auto path=root.filePath(name);QFile file(path);file.open(QIODevice::ReadOnly);originals[path]=QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);}
    s.catalog.openCatalog(root.filePath("photos"),root.filePath("catalog"),"QA-ONLY","2");
    s.documents.addFiles({QUrl::fromLocalFile(root.filePath("Note.md"))});
    for(auto service:{&s.library,&s.conversion})service->addInputs({QUrl::fromLocalFile(root.filePath("Note.md")),QUrl::fromLocalFile(root.filePath("Corrupt.pdf"))});
    s.inventory.create(QUrl::fromLocalFile(root.filePath("Inventory.ahinv")),{{"project","Synthetic chaos QA"},{"site","QA"}});
    s.transcription.initialize();
    struct State {int step=0,cleanup=0;QRandomGenerator rng;QElapsedTimer clock; qint64 last=0,maxGap=0;QJsonObject counts;explicit State(quint32 value):rng(value){clock.start();}};
    auto state=std::make_shared<State>(seed);auto timer=new QTimer(window);timer->setInterval(25);
    QObject::connect(timer,&QTimer::timeout,window,[=]() mutable {
        const auto elapsed=state->clock.elapsed();state->maxGap=qMax(state->maxGap,elapsed-state->last);state->last=elapsed;
        auto count=[&](const QString &name){state->counts[name]=state->counts[name].toInt()+1;};
        auto escape=[&]{QKeyEvent press(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier),release(QEvent::KeyRelease,Qt::Key_Escape,Qt::NoModifier);QCoreApplication::sendEvent(window,&press);QCoreApplication::sendEvent(window,&release);};
        if(state->step>=iterations){
            if(state->cleanup++==0){s.tasks.cancelAll();s.coordinates.cancel();s.documents.cancel();s.library.cancel();s.conversion.cancel();s.inventory.cancel();s.transcription.cancel();escape();}
            if(s.tasks.activeCount()>0&&state->cleanup<600)return;
            bool preserved=true;for(auto it=originals.cbegin();it!=originals.cend();++it){QFile file(it.key());preserved&=file.open(QIODevice::ReadOnly)&&QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256)==it.value();}
            timer->stop();window->grabWindow().save(root.filePath("final.png"));
            completed({{"ok",preserved&&s.tasks.activeCount()==0},{"steps",state->step},{"seed",qint64(seed)},{"elapsedMs",elapsed},{"maximumEventLoopGapMs",state->maxGap},{"originalsUnchanged",preserved},{"activeTasksAfterCleanup",s.tasks.activeCount()},{"actions",state->counts}});timer->deleteLater();return;
        }
        ++state->step;
        if(state->step%100==0){QFile progress(root.filePath("progress.json"));if(progress.open(QIODevice::WriteOnly|QIODevice::Truncate))progress.write(QJsonDocument(QJsonObject{{"step",state->step},{"elapsedMs",elapsed}}).toJson());}
        static const QStringList routes{"home","tools","projects","favorites","recent","activity","settings","about","tool/workbench.coordinates","tool/workbench.photo-catalog","tool/workbench.transcription","tool/workbench.document-summary","tool/workbench.inventory"};
        const auto choice=state->rng.bounded(24);
        switch(choice){
        case 0:case 1:case 2:case 3: s.hub.navigate(routes[state->rng.bounded(routes.size())]);count("navigate");break;
        case 4: s.hub.goBack();count("back");break;
        case 5: s.settings.setTheme(state->rng.bounded(2)?"dark":"light");window->resize(state->rng.bounded(2)?QSize(960,640):QSize(1280,820));count("theme-resize");break;
        case 6: QMetaObject::invokeMethod(window,"openCommandPalette");count("palette");break;
        case 7: escape();s.hub.dismissNotice();count("dismiss");break;
        case 8: QMetaObject::invokeMethod(window,"openNewProject");count("project-dialog");break;
        case 9: if(!s.coordinates.busy())s.coordinates.request({{"op","convert"},{"source","EPSG:4326"},{"target","EPSG:8908"},{"rows",QJsonArray{QJsonObject{{"id","=1+1"},{"x",state->step%2?"NaN":"-84"},{"y","10"}}}}});count("coordinate-valid-invalid");break;
        case 10: s.coordinates.cancel();count("coordinate-cancel");break;
        case 11: if(!s.catalog.busy()&&s.catalog.opened()&&!s.catalog.items().isEmpty())s.catalog.select(state->step%2?s.catalog.items().first().toMap()["id"].toString():"unknown-id");count("photo-select-invalid");break;
        case 12: if(!s.catalog.busy()&&!s.catalog.selection().isEmpty())s.catalog.propose({{"operation","2"},{"unitType","Pozo"},{"unit","7"},{"level","1"},{"view","../CON =1+1"}});count("photo-invalid-name");break;
        case 13: s.library.analyze();if(state->step%2)s.library.cancel();count("library-analyze-cancel");break;
        case 14: s.conversion.analyze();if(state->step%2)s.conversion.cancel();count("conversion-analyze-cancel");break;
        case 15: s.documents.generate();s.documents.cancel();count("summary-start-cancel");break;
        case 16: s.transcription.addFiles({QUrl::fromLocalFile(root.filePath("Corrupt.wav"))});s.transcription.start();s.transcription.cancel();count("transcription-corrupt-cancel");break;
        case 17: if(s.inventory.opened())s.inventory.add({{"operacion","2"},{"q_cer",state->step%2?4:-4},{"comentarios","=1+1"}});count("inventory-valid-invalid");break;
        case 18: if(s.inventory.opened())s.inventory.search("' OR 1=1 --",state->step%2);count("inventory-search");break;
        case 19: if(s.inventory.opened())s.inventory.exportFile(QUrl::fromLocalFile(root.filePath("exports/Inventory.csv")));count("inventory-export-collision");break;
        case 20: if(!s.coordinates.busy())s.coordinates.exportFile(QUrl::fromLocalFile(root.filePath("exports/Coordinates.csv")),"csv");count("coordinate-export-collision");break;
        case 21: s.hub.hashFile(QUrl::fromLocalFile(root.filePath("Note.md")));s.tasks.cancelAll();count("hash-cancel-all");break;
        case 22: s.hub.navigate("tool/not-a-tool");s.hub.createProject(QUrl::fromLocalFile(root.path()),"../escape");count("invalid-route-project");break;
        case 23: if(s.conversion.hasPlan())s.conversion.exportLibrary(QUrl::fromLocalFile(root.filePath("exports")),false);count("markdown-export");break;
        }
    });timer->start();
}
}
