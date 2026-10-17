#include "services/Store.h"
#include "services/Settings.h"
#include "services/Logger.h"
#include "services/Volumes.h"
#include "services/TaskManager.h"
#include "services/Registry.h"
#include "ui/Hub.h"
#include "ui/ToolFilter.h"
#include "ui/SymbolProvider.h"
#include "modules/catalog/CatalogController.h"
#include "modules/coordinates/CoordinateController.h"
#include "modules/transcription/TranscriptionController.h"
#include "modules/documents/DocumentController.h"
#include "modules/documents/DocumentLibraryController.h"
#include "modules/inventory/InventoryController.h"
#include "UiChaos.h"
#include "shared/UiNetworkPolicy.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QLineF>
#include <QQuickStyle>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QTranslator>
#include <QFontDatabase>
#include <QIcon>
#include <QMutex>
#include <QMutexLocker>
#include <QLockFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCryptographicHash>
#include <cstdio>

using namespace workbench;
namespace {
Logger *activeLogger=nullptr;
QStringList warnings;
QString warningRoute;
QMutex warningMutex;
QMutex sinkMutex;
void messages(QtMsgType type,const QMessageLogContext &,const QString &message){
    QString route;{QMutexLocker lock(&warningMutex);route=warningRoute;}
    {QMutexLocker lock(&sinkMutex);if(activeLogger)activeLogger->write(type==QtWarningMsg?"WARNING":type==QtDebugMsg?"DEBUG":type==QtInfoMsg?"INFO":"ERROR","qt.message",{{"message",message},{"route",route}});}
    if(type==QtWarningMsg||type==QtCriticalMsg){QMutexLocker lock(&warningMutex);if(warnings.size()<1000)warnings<<message;}
}
QStringList warningSnapshot(){QMutexLocker lock(&warningMutex);return warnings;}
class MessageCapture {
public:
    explicit MessageCapture(Logger &logger){QMutexLocker lock(&sinkMutex);activeLogger=&logger;m_previous=qInstallMessageHandler(messages);}
    ~MessageCapture(){qInstallMessageHandler(m_previous);QMutexLocker lock(&sinkMutex);activeLogger=nullptr;}
    MessageCapture(const MessageCapture &)=delete;
    MessageCapture &operator=(const MessageCapture &)=delete;
private:
    QtMessageHandler m_previous=nullptr;
};
void writeReport(const QString &path,const QJsonObject &data){
    if(path.isEmpty())return;QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile file(path);
    if(file.open(QIODevice::WriteOnly)){file.write(QJsonDocument(data).toJson());file.commit();}
}
QJsonObject checkCatalogZoom(QQuickWindow *window){
    auto page=window->findChild<QObject*>("catalogWorkspace");auto view=window->findChild<QQuickItem*>("catalogPreview");auto photo=window->findChild<QQuickItem*>("catalogPreviewImage");
    if(!page||!view||!photo)return {{"ok",false},{"error","preview missing"}};
    QJsonArray checks;bool ok=true;
    auto settle=[](){QEventLoop events;QTimer::singleShot(35,&events,&QEventLoop::quit);events.exec();};
    for(int angle:{0,90,180,270}){
        page->setProperty("turn",angle);page->setProperty("zoom",3.0);settle();
        view->setProperty("contentX",(view->property("contentWidth").toDouble()-view->width())/2);view->setProperty("contentY",(view->property("contentHeight").toDouble()-view->height())/2);
        const QPointF scene=view->mapToScene({view->width()*.57,view->height()*.54}).toPoint();const auto before=photo->mapFromScene(scene);const QPointF normalized(before.x()/photo->width(),before.y()/photo->height());
        QWheelEvent wheel(scene,window->mapToGlobal(scene.toPoint()),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QCoreApplication::sendEvent(window,&wheel);settle();
        const auto after=photo->mapFromScene(scene);const double drift=QLineF(normalized,{after.x()/photo->width(),after.y()/photo->height()}).length();
        const double pixelDrift=QLineF(scene,photo->mapToScene({normalized.x()*photo->width(),normalized.y()*photo->height()})).length();
        const bool anchored=pixelDrift<=1.0&&qAbs(page->property("zoom").toDouble()-3.6)<1e-6;
        QWheelEvent backwards(scene,window->mapToGlobal(scene.toPoint()),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::ControlModifier,Qt::NoScrollPhase,false);QCoreApplication::sendEvent(window,&backwards);QCoreApplication::processEvents();
        const bool reverse=qAbs(page->property("zoom").toDouble()-3.0)<1e-6;
        const bool horizontal=view->property("contentWidth").toDouble()>view->width()+5;
        const QString panProperty=horizontal?"contentX":"contentY";const double panBefore=view->property(panProperty.toLatin1()).toDouble();const QPointF movement=horizontal?QPointF(48,0):QPointF(0,48);
        auto mouse=[&](QEvent::Type type,const QPointF &pos,Qt::MouseButton button,Qt::MouseButtons buttons){QMouseEvent event(type,pos,window->mapToGlobal(pos.toPoint()),button,buttons,Qt::NoModifier);static quint64 timestamp=1000;event.setTimestamp(timestamp+=40);QCoreApplication::sendEvent(window,&event);settle();};
        mouse(QEvent::MouseButtonPress,scene,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,scene+movement/3,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseMove,scene+movement,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,scene+movement,Qt::LeftButton,Qt::NoButton);
        const bool dragged=qAbs(view->property(panProperty.toLatin1()).toDouble()-panBefore)>5;
        QMetaObject::invokeMethod(page,"fitView");QCoreApplication::processEvents();const bool fitted=page->property("turn").toInt()==angle&&qAbs(page->property("zoom").toDouble()-1)<1e-6&&qAbs(view->property("contentX").toDouble())<1e-6&&qAbs(view->property("contentY").toDouble())<1e-6;
        checks<<QJsonObject{{"rotation",angle},{"normalizedDrift",drift},{"screenPixelDrift",pixelDrift},{"toleranceScreenPixels",1.0},{"wheelAnchored",anchored},{"ctrlWheelReverse",reverse},{"dragPans",dragged},{"panBefore",panBefore},{"fitRetainsRotationAndResetsPan",fitted}};ok&=anchored&&reverse&&dragged&&fitted;
    }
    page->setProperty("turn",0);QMetaObject::invokeMethod(page,"fitView");return {{"ok",ok},{"checks",checks}};
}
}
int main(int argc,char **argv){
    QElapsedTimer startup;startup.start();QGuiApplication app(argc,argv);
    app.setOrganizationName("ADOCR");app.setApplicationName("archaeology-workbench");app.setApplicationVersion(WORKBENCH_VERSION);
    auto systemFont=QFontDatabase::systemFont(QFontDatabase::GeneralFont);
#ifdef Q_OS_WIN
    systemFont.setFamily(QFontDatabase::families().contains("Segoe UI Variable")?"Segoe UI Variable":"Segoe UI");
#endif
    app.setFont(systemFont);
    QQuickStyle::setStyle("Basic");QQuickWindow::setTextRenderType(QQuickWindow::QtTextRendering);
    QCommandLineParser parser;parser.setApplicationDescription("Modular archaeology workbench");parser.addHelpOption();parser.addVersionOption();
    for(const auto &name:{"data-dir","report","screenshot","route","theme","configure-tool","launch-tool","program","interpreter","export-icon","width","height","create-project","parent-folder","catalog-source","catalog-output","catalog-site","catalog-operation","catalog-project","coordinate-request","coordinate-export","coordinate-ui-options","ui-network-probe","document-request","document-export","library-request","library-ui-request","inventory-request","inventory-open"})
        parser.addOption(QCommandLineOption(QStringList{name},QString("Value for %1").arg(name),"value"));
    parser.addOption({"self-check","Check installation without opening a window"});parser.addOption({"smoke-test","Render UI and exit"});
    parser.addOption({"palette","Open command palette during UI verification"});
    parser.addOption({"catalog-first","Select the first photo during isolated UI verification"});
    parser.addOption({"catalog-grid","Show the photo browser during isolated UI verification"});
    parser.addOption({"catalog-zoom-check","Check real pointer wheel events on synthetic catalog preview"});
    parser.addOption({"coordinate-demo","Convert explicit synthetic example points during UI verification"});
    parser.addOption({"chaos-test","Exercise rapid UI actions with isolated synthetic data"});
    parser.addOption({"chaos-iterations","Number of rapid UI actions","number","1500"});
    parser.addOption({"document-library","Open the document library workspace during UI verification"});
    parser.addOption({"inventory-entry","Show the material entry form during isolated UI verification"});
    parser.addOption({"new-project-dialog","Open project creation preview during UI verification"});parser.process(app);
    QFile branding(":/resources/branding.json");branding.open(QIODevice::ReadOnly);const auto brand=QJsonDocument::fromJson(branding.readAll()).object();
    app.setApplicationDisplayName(brand.value("displayName").toString("ArchaeoHub"));
    SymbolProvider symbols;const QImage mark=symbols.requestImage("coordinates/0B7F6F",nullptr,QSize(128,128));app.setWindowIcon(QIcon(":/resources/app.ico"));
    if(parser.isSet("export-icon"))return mark.save(parser.value("export-icon"),"ICO")?0:1;
    const auto dataDirectory=parser.isSet("data-dir")?QFileInfo(parser.value("data-dir")).absoluteFilePath():QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    // Un perfil alternativo explícito del Hub debe mantener sus preferencias e historial de transcripción fuera del perfil anterior.
    if(parser.isSet("data-dir")&&!qEnvironmentVariableIsSet("GAMBOA_CONFIG_DIR"))
        qputenv("GAMBOA_CONFIG_DIR",QDir(dataDirectory).filePath("transcription/configuration").toUtf8());
    if(!QDir().mkpath(dataDirectory)){writeReport(parser.value("report"),{{"ok",false},{"code","profile.unavailable"}});return 12;}
    QLockFile profileLock(QDir(dataDirectory).filePath(".archaeohub.lock"));profileLock.setStaleLockTime(0);
    auto identity=QFileInfo(dataDirectory).canonicalFilePath();
#ifdef Q_OS_WIN
    identity=identity.toCaseFolded();
#endif
    const auto serverName="archaeohub-"+QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(),QCryptographicHash::Sha256).toHex().left(32));
    if(!profileLock.tryLock(0)){
        if(profileLock.error()!=QLockFile::LockFailedError){writeReport(parser.value("report"),{{"ok",false},{"code","profile.lock_unavailable"}});std::fputs("No se pudo proteger el perfil de ArchaeoHub. Revisa los permisos.\n",stderr);return 12;}
        const bool command=parser.isSet("self-check")||parser.isSet("smoke-test")||parser.isSet("chaos-test")||parser.isSet("coordinate-request")||parser.isSet("document-request")||parser.isSet("library-request")||parser.isSet("inventory-request")||parser.isSet("configure-tool")||parser.isSet("launch-tool")||parser.isSet("create-project");
        bool activated=false;
        if(!command){QLocalSocket socket;socket.connectToServer(serverName);if(socket.waitForConnected(1000)){socket.write("activate\n");activated=socket.waitForBytesWritten(1000);}}
        writeReport(parser.value("report"),{{"ok",!command&&activated},{"alreadyRunning",true},{"code","profile.in_use"}});
        std::fputs("ArchaeoHub ya esta abierto con este perfil.\n",stderr);return !command&&activated?0:12;
    }
    QLocalServer instanceServer;instanceServer.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(serverName);instanceServer.listen(serverName);
    try{
        Logger logger(dataDirectory);MessageCapture capture(logger);
        Store store(dataDirectory);Settings settings(dataDirectory);Volumes volumes(store);TaskManager tasks(store);
        Registry registry(store,volumes,dataDirectory);Hub hub(store,registry,volumes,tasks,logger,app.applicationDisplayName(),app.applicationVersion());
        QObject::connect(&hub,&Hub::changed,&app,[&hub]{QMutexLocker lock(&warningMutex);warningRoute=hub.route();});
        if(parser.isSet("theme"))settings.setTheme(parser.value("theme"));
        QTranslator translator;if(QLocale().language()==QLocale::English&&translator.load(":/translations/workbench_en.qm"))app.installTranslator(&translator);
        if(parser.isSet("configure-tool")||parser.isSet("self-check")||parser.isSet("launch-tool")||parser.isSet("create-project")){
            volumes.refresh();QEventLoop wait;QTimer timeout;timeout.setSingleShot(true);
            QObject::connect(&volumes,&Volumes::changed,&wait,[&]{if(!volumes.refreshing())wait.quit();});
            QObject::connect(&timeout,&QTimer::timeout,&wait,&QEventLoop::quit);timeout.start(12000);wait.exec();
            if(!volumes.hasSnapshot()){
                writeReport(parser.value("report"),{{"ok",false},{"error","volume.probe_timeout"},{"volumeProbeComplete",false}});return 8;
            }
            if(parser.isSet("configure-tool")){
                const auto error=registry.configure(parser.value("configure-tool"),parser.value("program"),parser.value("interpreter"));
                if(!error.isEmpty()){writeReport(parser.value("report"),{{"ok",false},{"error",error}});return 3;}
            }
            if(parser.isSet("self-check")){
                writeReport(parser.value("report"),{{"ok",registry.rowCount()>=1},{"qtVersion",qVersion()},{"schemaVersion",1},
                    {"registeredTools",registry.rowCount()},{"volumeProbeComplete",true},{"volumes",volumes.snapshot().size()},{"startupMs",startup.elapsed()}});
                return registry.rowCount()>=1?0:4;
            }
            if(parser.isSet("launch-tool")){
                const auto result=hub.openTool(parser.value("launch-tool"));writeReport(parser.value("report"),QJsonObject::fromVariantMap(result));
                return result["ok"].toBool()?0:6;
            }
            if(parser.isSet("create-project")){
                const bool started=hub.createProject(QUrl::fromLocalFile(parser.value("parent-folder")),parser.value("create-project"));
                if(started){
                    QEventLoop creation;QTimer deadline;deadline.setSingleShot(true);
                    QObject::connect(&hub,&Hub::changed,&creation,[&]{if(!hub.creatingProject())creation.quit();});
                    QObject::connect(&deadline,&QTimer::timeout,&creation,&QEventLoop::quit);deadline.start(30000);creation.exec();
                }
                const bool ok=started&&!hub.creatingProject()&&!hub.noticeError();
                writeReport(parser.value("report"),{{"ok",ok},{"message",hub.notice()},{"folders",hub.projectFolders().size()},{"projects",hub.projects().size()}});
                return ok?0:7;
            }
            return 0;
        }
        ToolFilter filter;filter.setSourceModel(&registry);
        catalog::CatalogController catalog(tasks,dataDirectory);
        documents::DocumentController documents(tasks,dataDirectory);
        documents::DocumentLibraryController documentLibrary(tasks,dataDirectory);
        documents::DocumentLibraryController documentConversion(tasks,dataDirectory,nullptr,11434,true);
        inventory::InventoryController inventory(tasks,dataDirectory);
        transcription::TranscriptionController transcription(tasks,dataDirectory,store.installation("workbench.transcription").value("program").toMap().value("absoluteHint").toString());
        coordinates::CoordinateController coordinates(tasks,dataDirectory);
        auto readLibraryRequest=[&](const QString &path){QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)return QJsonObject();return QJsonDocument::fromJson(file.readAll()).object();};
        auto addLibraryInputs=[&](const QJsonObject &request){QVariantList urls;for(const auto &path:request["paths"].toArray())urls<<QUrl::fromLocalFile(path.toString());(request["conversionOnly"].toBool()?documentConversion:documentLibrary).addInputs(urls);};
        const auto libraryUiRequest=parser.isSet("library-ui-request")?readLibraryRequest(parser.value("library-ui-request")):QJsonObject();
        const bool conversionUi=libraryUiRequest["conversionOnly"].toBool();
        if(parser.isSet("inventory-request")){
            QFile input(parser.value("inventory-request"));if(!input.open(QIODevice::ReadOnly)||input.size()>2*1024*1024)return 14;
            const auto request=QJsonDocument::fromJson(input.readAll()).object();QEventLoop operation;QTimer deadline;deadline.setSingleShot(true);bool ok=false;
            QObject::connect(&inventory,&inventory::InventoryController::completed,&operation,[&](bool succeeded){ok=succeeded;operation.quit();});QObject::connect(&deadline,&QTimer::timeout,&operation,&QEventLoop::quit);
            inventory.request(request);if(inventory.busy()){deadline.start(190000);operation.exec();}if(inventory.busy()){inventory.cancel();ok=false;}
            auto result=inventory.lastResult();result["ok"]=ok;result["message"]=inventory.message();writeReport(parser.value("report"),result);return ok?0:14;
        }
        if(parser.isSet("library-request")){
            const auto request=readLibraryRequest(parser.value("library-request"));addLibraryInputs(request);QEventLoop operation;QTimer deadline;deadline.setSingleShot(true);bool ok=false;
            auto &controller=request["conversionOnly"].toBool()?documentConversion:documentLibrary;
            QObject::connect(&controller,&documents::DocumentLibraryController::completed,&operation,[&](bool succeeded){ok=succeeded;operation.quit();});QObject::connect(&deadline,&QTimer::timeout,&operation,&QEventLoop::quit);
            controller.analyze();if(controller.busy()){deadline.start(250000);operation.exec();}if(controller.busy()){controller.cancel();ok=false;}
            if(ok&&request["research"].toBool()){
                ok=false;controller.enrich(request["model"].toString());if(controller.busy()){deadline.start(3600000);operation.exec();}if(controller.busy()){controller.cancel();ok=false;}
            }
            if(ok&&!request["destination"].toString().isEmpty()){
                ok=false;controller.exportLibrary(QUrl::fromLocalFile(request["destination"].toString()),request["automaticLinks"].toBool(true));if(controller.busy()){deadline.start(250000);operation.exec();}if(controller.busy()){controller.cancel();ok=false;}
            }
            writeReport(parser.value("report"),{{"ok",ok},{"ready",controller.readyCount()},{"failed",controller.failedCount()},{"research",controller.hasAnalysis()},{"connections",QJsonArray::fromVariantList(controller.connections())},{"topics",QJsonArray::fromVariantList(controller.topics())},{"rows",QJsonArray::fromVariantList(controller.rows())},{"folder",controller.resultFolder()},{"message",controller.message()}});return ok?0:13;
        }
        if(parser.isSet("document-request")){
            QFile input(parser.value("document-request"));if(!input.open(QIODevice::ReadOnly)||input.size()>65536)return 12;
            const auto request=QJsonDocument::fromJson(input.readAll()).object();QEventLoop models;QTimer deadline;deadline.setSingleShot(true);
            QObject::connect(&documents,&documents::DocumentController::changed,&models,&QEventLoop::quit);
            QObject::connect(&deadline,&QTimer::timeout,&models,&QEventLoop::quit);documents.initialize();deadline.start(15000);models.exec();
            if(!request["model"].toString().isEmpty())documents.setModel(request["model"].toString());
            QVariantList files;for(const auto &file:request["paths"].toArray())files<<QUrl::fromLocalFile(file.toString());documents.addFiles(files);
            bool succeeded=false;QEventLoop operation;
            QObject::connect(&documents,&documents::DocumentController::completed,&operation,[&](bool ok){succeeded=ok;operation.quit();});
            QObject::connect(&deadline,&QTimer::timeout,&operation,&QEventLoop::quit);documents.generate(request["detailed"].toBool());
            if(documents.busy()){deadline.start(3600000);operation.exec();}if(documents.busy())documents.cancel();
            if(succeeded&&parser.isSet("document-export"))documents.exportSummary(QUrl::fromLocalFile(parser.value("document-export")));
            writeReport(parser.value("report"),{{"ok",succeeded},{"model",documents.model()},{"summary",documents.summary()},{"message",documents.message()},{"warnings",QJsonArray::fromStringList(documents.warnings())},{"files",documents.files().size()}});return succeeded?0:12;
        }
        if(parser.isSet("coordinate-request")){
            QFile input(parser.value("coordinate-request"));if(!input.open(QIODevice::ReadOnly)||input.size()>8*1024*1024)return 11;
            const auto request=QJsonDocument::fromJson(input.readAll()).object();QEventLoop operation;QTimer deadline;deadline.setSingleShot(true);
            QObject::connect(&coordinates,&coordinates::CoordinateController::changed,&operation,[&]{if(!coordinates.busy())operation.quit();});
            QObject::connect(&deadline,&QTimer::timeout,&operation,&QEventLoop::quit);
            const bool started=coordinates.request(request);if(started&&coordinates.busy()){deadline.start(190000);operation.exec();}
            if(parser.isSet("coordinate-export"))coordinates.exportFile(QUrl::fromLocalFile(parser.value("coordinate-export")),"csv");
            const bool ok=started&&!coordinates.busy()&&!coordinates.error();auto report=coordinates.lastResult();report["ok"]=ok;report["message"]=coordinates.message();report["code"]=coordinates.technicalCode();writeReport(parser.value("report"),report);return ok?0:11;
        }
        LocalUiNetworkFactory uiNetworkPolicy;QQmlApplicationEngine engine;engine.setNetworkAccessManagerFactory(&uiNetworkPolicy);engine.addImageProvider("symbols",new SymbolProvider);
        if(parser.isSet("ui-network-probe")){
            const QUrl probe(parser.value("ui-network-probe"));if(!parser.isSet("smoke-test")||probe.scheme()!="http"||probe.host()!="127.0.0.1")return 11;
            auto reply=engine.networkAccessManager()->get(QNetworkRequest(probe));QObject::connect(reply,&QNetworkReply::finished,reply,&QObject::deleteLater);
        }
        engine.rootContext()->setContextProperty("hub",&hub);engine.rootContext()->setContextProperty("preferences",&settings);
        engine.rootContext()->setContextProperty("toolModel",&registry);engine.rootContext()->setContextProperty("toolFilter",&filter);
        engine.rootContext()->setContextProperty("volumes",&volumes);engine.rootContext()->setContextProperty("tasks",&tasks);
        engine.rootContext()->setContextProperty("catalog",&catalog);engine.rootContext()->setContextProperty("coordinates",&coordinates);
        engine.rootContext()->setContextProperty("transcription",&transcription);
        engine.rootContext()->setContextProperty("documents",&documents);
        engine.rootContext()->setContextProperty("documentLibrary",&documentLibrary);
        engine.rootContext()->setContextProperty("documentConversion",&documentConversion);
        engine.rootContext()->setContextProperty("inventory",&inventory);
        if(parser.isSet("route"))hub.navigate(parser.value("route"));
        engine.load(QUrl("qrc:/ui/Main.qml"));
        if(engine.rootObjects().isEmpty()){writeReport(parser.value("report"),{{"ok",false},{"qmlWarnings",QJsonArray::fromStringList(warningSnapshot())}});return 2;}
        auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        window->setIcon(app.windowIcon());
        QObject::connect(&instanceServer,&QLocalServer::newConnection,window,[&]{while(instanceServer.hasPendingConnections()){
            auto socket=instanceServer.nextPendingConnection();socket->setReadBufferSize(64);
            auto receive=[socket,window]{if(!socket->canReadLine())return;const auto message=socket->readLine(64);if(message=="activate\n"){if(window->visibility()==QWindow::Minimized)window->showNormal();window->raise();window->requestActivate();}socket->disconnectFromServer();};
            QObject::connect(socket,&QLocalSocket::readyRead,window,receive);QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);QTimer::singleShot(1500,socket,&QLocalSocket::disconnectFromServer);receive();
        }});
        if(parser.isSet("width"))window->setWidth(parser.value("width").toInt());if(parser.isSet("height"))window->setHeight(parser.value("height").toInt());
        const auto readyMs=startup.elapsed();logger.write("INFO","app.ready",{{"version",app.applicationVersion()},{"startupMs",readyMs}});
        if(parser.isSet("chaos-test")){
            if(!parser.isSet("data-dir")||!parser.isSet("report"))return 11;
            QTimer::singleShot(200,&app,[&]{startUiChaos(window,QDir(dataDirectory).filePath("chaos-fixtures"),qBound(100,parser.value("chaos-iterations").toInt(),5000),1374,
                {hub,settings,tasks,catalog,coordinates,transcription,documents,documentLibrary,documentConversion,inventory},[&](QJsonObject report){report["qmlWarnings"]=QJsonArray::fromStringList(warningSnapshot());report["ok"]=report["ok"].toBool()&&warningSnapshot().isEmpty();writeReport(parser.value("report"),report);app.exit(report["ok"].toBool()?0:5);});});
        }
        if(parser.isSet("palette"))QTimer::singleShot(200,&app,[window]{QMetaObject::invokeMethod(window,"openCommandPalette");});
        if(parser.isSet("new-project-dialog"))QTimer::singleShot(200,&app,[window]{QMetaObject::invokeMethod(window,"openNewProject");});
        if(parser.isSet("coordinate-ui-options")){
            if(!parser.isSet("smoke-test")||hub.route()!="tool/workbench.coordinates")return 11;
            QFile input(parser.value("coordinate-ui-options"));if(!input.open(QIODevice::ReadOnly)||input.size()>4096)return 11;
            const auto options=QJsonDocument::fromJson(input.readAll()).object();
            QTimer::singleShot(150,&app,[window,options]{
                if(auto page=window->findChild<QObject*>("coordinateWorkspace")){
                    page->setProperty("batch",options["batch"].toBool());
                    for(const auto &name:{"coordinateInputFormat","coordinateOutputFormat"})
                        if(auto field=page->findChild<QObject*>(name))field->setProperty("currentIndex",qBound(0,options[QString::fromLatin1(name)].toInt(),2));
                }
            });
            if(options["crsPicker"].toBool()){
                auto timer=new QTimer(window);timer->setInterval(100);
                QObject::connect(timer,&QTimer::timeout,window,[window,options,timer,&coordinates]{
                    if(coordinates.busy())return;
                    timer->stop();timer->deleteLater();
                    if(auto page=window->findChild<QObject*>("coordinateWorkspace")){
                        if(auto field=page->findChild<QObject*>("coordinateCrsGroup"))field->setProperty("currentIndex",qBound(0,options["crsGroup"].toInt(),8));
                        if(auto field=page->findChild<QObject*>("coordinateCrsQuery"))field->setProperty("text",options["crsQuery"].toString());
                        QMetaObject::invokeMethod(page,"pick",Q_ARG(QVariant,QVariant("source")));
                    }
                });timer->start();
            }
        }
        if(parser.isSet("document-library")||parser.isSet("library-ui-request"))QTimer::singleShot(50,&app,[&]{if(auto page=window->findChild<QObject*>("documentWorkspace"))page->setProperty("currentTab",conversionUi?2:1);});
        if(parser.isSet("library-ui-request"))QTimer::singleShot(150,&app,[&]{
            addLibraryInputs(libraryUiRequest);
            if(libraryUiRequest["research"].toBool()){
                auto started=std::make_shared<bool>(false);
                QObject::connect(&documentLibrary,&documents::DocumentLibraryController::completed,&app,[&,started](bool ok){
                    if(!ok)return;if(!*started){*started=true;documentLibrary.enrich(libraryUiRequest["model"].toString());}
                    else if(auto page=window->findChild<QObject*>("documentLibraryWorkspace"))QMetaObject::invokeMethod(page,"showPreview",Q_ARG(QVariant,QVariant("documents")),Q_ARG(QVariant,QVariant(0)));
                });
            }
            (conversionUi?documentConversion:documentLibrary).analyze();
        });
        if(parser.isSet("inventory-open"))QTimer::singleShot(100,&app,[&]{inventory.open(QUrl::fromLocalFile(parser.value("inventory-open")));});
        if(parser.isSet("inventory-entry"))QTimer::singleShot(300,&app,[&]{if(auto page=window->findChild<QObject*>("inventoryWorkspace"))QMetaObject::invokeMethod(page,"openEntry");});
        if(parser.isSet("catalog-project"))QTimer::singleShot(100,&app,[&]{
            auto page=window->findChild<QObject*>("catalogWorkspace");
            if(page){if(auto field=page->findChild<QObject*>("catalogSourceField"))field->setProperty("text",parser.value("catalog-source"));QMetaObject::invokeMethod(page,"loadCatalog",Q_ARG(QVariant,QVariant(QUrl::fromLocalFile(parser.value("catalog-project")))));}
        });
        if(parser.isSet("catalog-source")&&!parser.isSet("catalog-project")){
            auto firstSelected=std::make_shared<bool>(false);QObject::connect(&catalog,&catalog::CatalogController::completed,&app,[&,firstSelected]{
                if(parser.isSet("catalog-first")&&!*firstSelected&&catalog.opened()&&!catalog.items().isEmpty()&&!catalog.busy()){*firstSelected=true;catalog.select(catalog.items().first().toMap()["id"].toString());}
            });
            QTimer::singleShot(100,&app,[&]{catalog.openCatalog(parser.value("catalog-source"),parser.value("catalog-output"),parser.value("catalog-site"),parser.value("catalog-operation"));});
        }
        if(parser.isSet("catalog-grid"))QTimer::singleShot(200,&app,[window]{if(auto page=window->findChild<QObject*>("catalogWorkspace"))page->setProperty("browseOnly",true);});
        if(parser.isSet("coordinate-demo")){
            auto seeded=std::make_shared<bool>(false);QObject::connect(&coordinates,&coordinates::CoordinateController::changed,&app,[&,seeded]{
                if(!*seeded&&!coordinates.busy()&&!coordinates.sourceInfo().isEmpty()){*seeded=true;coordinates.example();coordinates.convertBatch({{"header",true},{"delimiter","\t"},{"xColumn",1},{"yColumn",2},{"idColumn",0},{"precision",3}});}
            });
        }
        auto smokeAttempts=std::make_shared<int>(0);auto smokeSettled=std::make_shared<bool>(false);auto smokeTimer=new QTimer(&app);smokeTimer->setInterval(1700);
        if(parser.isSet("smoke-test")){QObject::connect(smokeTimer,&QTimer::timeout,&app,[&,smokeAttempts,smokeSettled,smokeTimer]{
            if((catalog.busy()||coordinates.busy()||documentLibrary.busy()||documentConversion.busy()||inventory.busy()||(hub.route()=="tool/workbench.transcription"&&!transcription.ready()))&&++*smokeAttempts<(libraryUiRequest["research"].toBool()?2000:120))return;smokeTimer->stop();
            if(!*smokeSettled){*smokeSettled=true;smokeTimer->start(400);return;}
            QJsonArray buttonStyles;
            for(auto button:window->findChildren<QObject*>("actionButton")){
                auto caption=button->findChild<QObject*>("actionCaption");
                buttonStyles.append(QJsonObject{{"text",button->property("text").toString()},{"variant",button->property("variant").toString()},
                    {"foreground",button->property("foreground").value<QColor>().name()},{"enabled",button->property("enabled").toBool()},
                    {"caption",caption?caption->property("color").value<QColor>().name():QString()}});
            }
            QJsonArray glassSurfaces;
            for(auto item:window->findChildren<QQuickItem*>())if(item->isVisible()&&item->property("isGlassSurface").toBool())
                glassSurfaces.append(QJsonObject{{"frosted",item->property("frosted").toBool()},{"effectReady",item->property("effectReady").toBool()},
                    {"opaqueFallback",item->property("opaqueFallback").toBool()},{"popup",item->property("popupSurface").toBool()},
                    {"bounds",QJsonArray{item->mapToScene(QPointF()).x(),item->mapToScene(QPointF()).y(),item->width(),item->height()}}});
            const auto zoomCheck=parser.isSet("catalog-zoom-check")?checkCatalogZoom(window):QJsonObject{{"ok",true}};
            QJsonObject coordinateUi;
            if(auto page=window->findChild<QObject*>("coordinateWorkspace")){
                QVariant options;QMetaObject::invokeMethod(page,"options",Q_RETURN_ARG(QVariant,options));
                coordinateUi={{"source",QJsonObject::fromVariantMap(coordinates.sourceInfo())},{"target",QJsonObject::fromVariantMap(coordinates.targetInfo())},{"options",QJsonObject::fromVariantMap(options.toMap())}};
                if(auto action=page->findChild<QQuickItem*>("coordinateConvert")){
                    const auto rectangle=action->mapRectToScene(QRectF(0,0,action->width(),action->height()));
                    coordinateUi["convertButtonWithinWindow"]=action->isVisible()&&QRectF(0,0,window->width(),window->height()).contains(rectangle);
                    coordinateUi["convertButtonBottom"]=rectangle.bottom();
                }
                coordinateUi["pageHeight"]=page->property("height").toDouble();
                if(auto picker=page->findChild<QObject*>("coordinateCrsPicker"))coordinateUi["picker"]=QJsonObject{{"visible",picker->property("visible").toBool()},
                    {"total",coordinates.searchTotal()},{"offset",coordinates.searchOffset()},{"results",QJsonArray::fromVariantList(coordinates.searchResults())}};
                for(const auto &name:{"coordinateInputFormat","coordinateOutputFormat","coordinateInputUnit","coordinateOutputUnit","coordinateInputHint"})
                    if(auto item=page->findChild<QQuickItem*>(name))coordinateUi[name]=QJsonObject{{"visible",item->isVisible()},{"text",item->property("text").toString()},{"currentText",item->property("currentText").toString()},{"width",item->width()}};
            }
            bool saved=zoomCheck["ok"].toBool();if(parser.isSet("screenshot")){const auto path=parser.value("screenshot");QDir().mkpath(QFileInfo(path).absolutePath());saved=window->grabWindow().save(path)&&saved;}
            const bool moduleReady=!catalog.busy()&&!coordinates.busy()&&!documentLibrary.busy()&&!documentConversion.busy()&&!inventory.busy()&&(!parser.isSet("inventory-open")||inventory.opened())&&(!parser.isSet("library-ui-request")||(conversionUi?documentConversion.hasPlan():documentLibrary.hasPlan()))&&(!libraryUiRequest["research"].toBool()||documentLibrary.hasAnalysis())&&(!parser.isSet("catalog-source")||catalog.opened())&&(!parser.isSet("catalog-project")||catalog.opened())&&(hub.route()!="tool/workbench.transcription"||transcription.ready());
            writeReport(parser.value("report"),{{"ok",saved&&moduleReady&&warningSnapshot().isEmpty()},{"startupMs",readyMs},{"route",hub.route()},
                {"qmlWarnings",QJsonArray::fromStringList(warningSnapshot())},{"glassSurfaces",glassSurfaces},{"graphicsApi",int(window->rendererInterface()->graphicsApi())},{"uiNetworkRequestsBlocked",uiNetworkPolicy.blocked->load()},{"buttonStyles",buttonStyles},{"coordinateUi",coordinateUi},{"catalogZoom",zoomCheck},{"screenshotSaved",saved},{"windowIconSet",!window->icon().isNull()},{"transcriptionReady",transcription.ready()},{"inventoryOpened",inventory.opened()},{"inventoryRecords",inventory.state().value("stats").toMap().value("records").toInt()},{"libraryReady",documentLibrary.readyCount()},{"libraryTopics",documentLibrary.topics().size()},{"catalogOpened",catalog.opened()},{"catalogSite",catalog.state().value("site").toString()}});app.exit(saved&&moduleReady&&warningSnapshot().isEmpty()?0:5);
        });smokeTimer->start();}
        return app.exec();
    }catch(const std::exception &error){

        writeReport(parser.value("report"),{{"ok",false},{"error",QString::fromUtf8(error.what())}});
        if(parser.isSet("self-check")||parser.isSet("smoke-test")||parser.isSet("configure-tool")||parser.isSet("launch-tool")||parser.isSet("create-project"))return 10;
        QQmlApplicationEngine errorUi;errorUi.rootContext()->setContextProperty("startupError",QString::fromUtf8(error.what()));
        errorUi.loadData("import QtQuick\nimport QtQuick.Controls\nApplicationWindow { visible: true; width: 680; height: 320; title: qsTr(\"ArchaeoHub · Inicio\"); Column { anchors.fill: parent; anchors.margins: 32; spacing: 20; Label { text: qsTr(\"No se pudo abrir el espacio de trabajo\"); font.pixelSize: 22 } Text { width: parent.width; wrapMode: Text.Wrap; text: startupError } Label { text: qsTr(\"Se conservaron los datos existentes. Revisa permisos y espacio disponible.\"); wrapMode: Text.Wrap; width: parent.width } } }");
        return errorUi.rootObjects().isEmpty()?10:app.exec();
    }
}
