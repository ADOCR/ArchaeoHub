#pragma once
#include <QSortFilterProxyModel>
#include "services/Registry.h"
namespace workbench {
class ToolFilter final : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY filterChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY filterChanged)
public:
    explicit ToolFilter(QObject *parent=nullptr):QSortFilterProxyModel(parent){setDynamicSortFilter(true);}
    QString mode() const{return m_mode;}
    QString query() const{return m_query;}
    void setMode(const QString &value){if(m_mode!=value){m_mode=value;invalidateFilter();emit filterChanged();}}
    void setQuery(const QString &value){if(m_query!=value){m_query=value;invalidateFilter();emit filterChanged();}}
signals:
    void filterChanged();
protected:
    bool filterAcceptsRow(int row,const QModelIndex &parent) const override {
        const auto index=sourceModel()->index(row,0,parent);
        if(m_mode=="favorites"&&!sourceModel()->data(index,Registry::FavoriteRole).toBool())return false;
        if(m_mode=="recent"&&sourceModel()->data(index,Registry::LastUsedRole).toString().isEmpty())return false;
        const auto text=sourceModel()->data(index,Registry::NameRole).toString()+" "+sourceModel()->data(index,Registry::DescriptionRole).toString()+" "+sourceModel()->data(index,Registry::CategoryRole).toString();
        return text.contains(m_query,Qt::CaseInsensitive);
    }
private:
    QString m_mode,m_query;
};
}
