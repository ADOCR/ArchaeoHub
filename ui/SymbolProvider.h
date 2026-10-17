#pragma once
#include <QQuickImageProvider>
namespace workbench {
class SymbolProvider final : public QQuickImageProvider {
public:
    SymbolProvider():QQuickImageProvider(QQuickImageProvider::Image){}
    QImage requestImage(const QString &id,QSize *size,const QSize &requestedSize) override;
};
}
