#include <QtTest>
#include <cmath>
#include "modules/coordinates/CoordinateText.h"
using namespace workbench::coordinates;
class CoordinateTextContracts final:public QObject {
    Q_OBJECT
private slots:
    void explicitDecimalAndFiniteNumbers(){
        QCOMPARE(parseNumber("-84,125",',').value,-84.125);
        QCOMPARE(parseNumber(" 1.25e3 ",'.').value,1250.0);
        for(const auto &text:{"NaN","inf","1e999","1 000","1,000.5","--1",""})QVERIFY(!parseNumber(text,'.'));
        QVERIFY(!parseNumber("84,1",'.'));QVERIFY(!parseNumber("84.1",','));
    }
    void hemisphereAndAxes(){
        QCOMPARE(parseAngle("84° 30′ 0″ W",true,"DMS",'.').value,-84.5);
        QCOMPARE(parseAngle("O 84 30",true,"DM",'.').value,-84.5);
        QCOMPARE(parseAngle("10 15 N",false,"DM",'.').value,10.25);
        QCOMPARE(parseAngle("84 W",false,"DD",'.').error,QString("input.axis_hemisphere"));
        QCOMPARE(parseAngle("-10 N",false,"DD",'.').error,QString("input.sign_hemisphere"));
        QCOMPARE(parseAngle("+10 S",false,"DD",'.').error,QString("input.sign_hemisphere"));
        QVERIFY(!parseAngle("10 60",false,"DM",'.'));QVERIFY(!parseAngle("90 0 0.1 N",false,"DMS",'.'));
        QVERIFY(!parseAngle("180.1",true,"DD",'.'));QVERIFY(!parseAngle("84.1 20",true,"DM",'.'));
        QVERIFY(!parseAngle("84 10.5 20",true,"DMS",'.'));
    }
    void roundingCarriesAndNegativeZero(){
        QCOMPARE(formatAngle(9.99999999,false,"DMS",3),QString::fromUtf8("10° 0′ 0.000″ N"));
        QCOMPARE(formatAngle(-0.0,false,"DMS",2),QString::fromUtf8("0° 0′ 0.00″ S"));
        QVERIFY(std::signbit(parseAngle("0 S",false,"DD",'.').value));
    }
    void formattingRoundTrips(){
        // Contrato de cuantización; evita duplicar el formateador de producción.
        for(int i=-720;i<=720;++i){const double angle=i/8.0;
            for(const auto &format:{"DD","DM","DMS"}){
                const auto text=formatAngle(angle,false,format,5);const auto parsed=parseAngle(text,false,format,'.');
                QVERIFY2(parsed,qPrintable(text));QVERIFY(std::abs(parsed.value-angle)<0.000006);
                QCOMPARE(formatAngle(parsed.value,false,format,5),text);
            }
        }
    }
    void quotedCsvAndEmptyFields(){
        auto table=parseTable(QString::fromUtf8("\xEF\xBB\xBFid;x;y;notes\r\nA;-84,1;10,2;\"contexto; con \"\"comillas\"\"\nsegunda línea\"\r\nB;;9;\r\n"),';');
        QVERIFY(table);QCOMPARE(table.rows.size(),3);QCOMPARE(table.rows[1][3],QString::fromUtf8("contexto; con \"comillas\"\nsegunda línea"));QCOMPARE(table.rows[2][1],QString());QCOMPARE(table.rows[2][3],QString());
        QVERIFY(!parseTable("a,\"sin cerrar",','));QVERIFY(!parseTable("a,\"b\"c",','));QVERIFY(!parseTable("a|b",'|'));
    }
    void csvSerializationRoundTrip(){
        const QStringList originals{QString(),"A;B","x,y",QString::fromUtf8("ñ\ncontexto"),"literal \"texto\"","-84.0"};
        QStringList fields;for(const auto &value:originals)fields<<quoteCsv(value);
        const auto parsed=parseTable(fields.join(','),',');QVERIFY(parsed);QCOMPARE(parsed.rows.first(),originals);
    }
    void csvFormulaTextIsNeutralizedWithoutChangingCoordinates(){
        for(const auto &value:QStringList{"=1+1","\t=1+1","+SUM(1)","-SUM(1)","@SUM(1)",QString::fromUtf8("＝1+1")}){
            auto table=parseTable(quoteCsv(value,true),',');QVERIFY(table);QCOMPARE(table.rows[0][0],"'"+value);
            QCOMPARE(parseTable(quoteCsv(value,false),',').rows[0][0],value);
        }
        for(const auto &value:{"-84.125","+10.25","-1e2","10.0"})QCOMPARE(parseTable(quoteCsv(value,true),',').rows[0][0],QString(value));
    }
};
QTEST_APPLESS_MAIN(CoordinateTextContracts)
#include "coordinate_text_tests.moc"
