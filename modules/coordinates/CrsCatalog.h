#pragma once
#include <QHash>
#include <QStringList>

namespace workbench::coordinates {
// Sólo identificadores. Las definiciones, unidades, ejes y transformaciones proceden siempre de PROJ.
// Revisado contra la base EPSG v12.029 incluida; consulta docs/CRS_REVIEW.md.
inline const QHash<QString,QStringList> &crsGroups(){
    static const auto groups=[] {
        QHash<QString,QStringList> result{
            {"recommended",{"4326","5365","5367","8907","8908","8909","8910","5451","5456","5457","32616","32617",
                            "3857","3395","4267","4269","4674","4248","4618","4686","5340","9377","32717","32718","32719"}},
            {"costa-rica",{"4326","5365","5367","8907","8908","8909","8910","5451","5456","5457","32616","32617"}},
            {"geographic",{"4326","5365","8907","5451","5467","4267","4269","4674","4248","4618","4686","5340","4258","4277","4230"}},
            {"central-america",{"5451","5456","5457","5459","5559","5460","5461","5462","5467","5469","5472",
                               "32615","32616","32617","32618","4267","4269","26715","26716","26717","26718"}},
            {"south-america",{"4674","4248","4618","4686","5340","9377","3114","3115","3116","3117","3118",
                             "5343","5344","5345","5346","5347","5348","5349","5396","6210","6211","9265","24877","24878","24879"}},
            {"world",{"4326","3857","3395","4258","25829","25830","25831","27700","2154","5070"}},
            {"utm-north",{}},{"utm-south",{}}
        };
        for(int zone=1;zone<=60;++zone){
            result["utm-north"].append(QString::number(32600+zone));
            result["utm-south"].append(QString::number(32700+zone));
        }
        for(int code=31965;code<=31985;++code)result["south-america"].append(QString::number(code));
        return result;
    }();
    return groups;
}
}
