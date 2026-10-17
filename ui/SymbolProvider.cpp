#include "SymbolProvider.h"
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>

namespace workbench {
QImage SymbolProvider::requestImage(const QString &id,QSize *size,const QSize &requestedSize){
    const auto parts=id.split('/');const auto name=parts.value(0);QColor color("#"+parts.value(1,"15171a"));if(!color.isValid())color=Qt::black;
    const auto length=qBound(24,requestedSize.width()>0?requestedSize.width():64,256);QImage image(length,length,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
    QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.scale(length/24.0,length/24.0);p.setPen(QPen(color,1.6,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
    auto line=[&](qreal x1,qreal y1,qreal x2,qreal y2){p.drawLine(QPointF(x1,y1),QPointF(x2,y2));};
    if(name=="home"){p.drawPolyline(QPolygonF{{3,10},{12,3},{21,10}});p.drawPolyline(QPolygonF{{5,9},{5,21},{10,21},{10,15},{14,15},{14,21},{19,21},{19,9}});}
    else if(name=="transcription"){for(int i=0;i<5;++i){const int heights[]={7,13,18,11,5};line(4+i*4,12-heights[i]/2.0,4+i*4,12+heights[i]/2.0);}}
    else if(name=="tools"||name=="catalog"){for(int x:{4,13})for(int y:{4,13})p.drawRoundedRect(QRectF(x,y,7,7),1.5,1.5);}
    else if(name=="folder"||name=="projects"){p.drawPolyline(QPolygonF{{3,7},{3,20},{21,20},{21,7},{12,7},{10,4},{3,4},{3,7}});}
    else if(name=="star"){QPolygonF star;for(int i=0;i<10;++i){const auto a=-1.57079632679+i*3.14159265359/5;const auto r=i%2?4.2:9.2;star<<QPointF(12+std::cos(a)*r,12+std::sin(a)*r);}p.drawPolygon(star);}
    else if(name=="clock"){p.drawEllipse(QRectF(3,3,18,18));line(12,6,12,12);line(12,12,16,14);}
    else if(name=="activity"){p.drawPolyline(QPolygonF{{2,13},{6,13},{9,5},{13,20},{16,10},{18,13},{22,13}});}
    else if(name=="settings"){for(int y:{6,12,18})line(3,y,21,y);for(auto point:{QPointF(8,6),QPointF(16,12),QPointF(10,18)}){p.setBrush(color);p.drawEllipse(point,2,2);}}
    else if(name=="search"){p.drawEllipse(QRectF(4,3,13,13));line(15,15,21,21);}
    else if(name=="plus"){line(12,5,12,19);line(5,12,19,12);}
    else if(name=="close"){line(6,6,18,18);line(18,6,6,18);}
    else if(name=="check"){p.drawPolyline(QPolygonF{{4,12},{9,17},{20,6}});}
    else if(name=="arrow"){line(4,12,20,12);p.drawPolyline(QPolygonF{{14,6},{20,12},{14,18}});}
    else if(name=="back"){line(4,12,20,12);p.drawPolyline(QPolygonF{{10,6},{4,12},{10,18}});}
    else if(name=="chevron"){p.drawPolyline(QPolygonF{{7,9},{12,14},{17,9}});}
    else if(name=="disk"){p.drawRoundedRect(QRectF(3,5,18,14),3,3);line(3,14,21,14);p.drawEllipse(QPointF(17,16.5),.5,.5);}
    else if(name=="document"){p.drawPolyline(QPolygonF{{6,3},{15,3},{19,7},{19,21},{6,21},{6,3}});line(9,11,16,11);line(9,15,16,15);}
    else if(name=="info"){p.drawEllipse(QRectF(3,3,18,18));line(12,10,12,17);p.drawEllipse(QPointF(12,7),.4,.4);}
    else if(name=="command"){p.drawRoundedRect(QRectF(3,3,6,6),2,2);p.drawRoundedRect(QRectF(15,3,6,6),2,2);p.drawRoundedRect(QRectF(3,15,6,6),2,2);p.drawRoundedRect(QRectF(15,15,6,6),2,2);p.drawRect(QRectF(8,8,8,8));}
    else if(name=="warning"){p.drawPolygon(QPolygonF{{12,3},{22,21},{2,21}});line(12,9,12,14);p.drawEllipse(QPointF(12,17),.4,.4);}
    else{p.drawRoundedRect(QRectF(4,4,16,16),3,3);line(12,1,12,8);line(12,16,12,23);line(1,12,8,12);line(16,12,23,12);}
    if(size)*size=image.size();return image;
}
}
