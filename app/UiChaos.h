#pragma once
#include "ui/Hub.h"
#include "services/Settings.h"
#include "modules/catalog/CatalogController.h"
#include "modules/coordinates/CoordinateController.h"
#include "modules/transcription/TranscriptionController.h"
#include "modules/documents/DocumentController.h"
#include "modules/documents/DocumentLibraryController.h"
#include "modules/inventory/InventoryController.h"
#include <QQuickWindow>
#include <functional>
namespace workbench {
struct ChaosServices {
    Hub &hub;Settings &settings;TaskManager &tasks;
    catalog::CatalogController &catalog;coordinates::CoordinateController &coordinates;
    transcription::TranscriptionController &transcription;documents::DocumentController &documents;
    documents::DocumentLibraryController &library,&conversion;inventory::InventoryController &inventory;
};
void startUiChaos(QQuickWindow *window,const QString &directory,int iterations,quint32 seed,
                  ChaosServices services,std::function<void(QJsonObject)> completed);
}
