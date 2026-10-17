#pragma once
#include <QString>
#include <QVariantList>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
class Parakeet {
public:
    explicit Parakeet(const QString &folder);
    ~Parakeet();
    QVariantList transcribe(const std::vector<float> &audio, std::atomic_bool &cancel,
                            const std::function<void(int,const QVariantList&)> &update);
private:
    struct Impl; std::unique_ptr<Impl> p;
};
