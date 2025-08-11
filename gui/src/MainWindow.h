#pragma once
#include <QMainWindow>
#include <QTableView>
#include <QComboBox>
#include <QSplitter>

// ВАЖЛИВО: явні інклуди класів Charts
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "DepBridge.h"
#include "ParamTableModel.h"

class QAction;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);

private slots:
    void onOpenFile();
    void onExportCsv();
    void onMaterialChanged(const QString&);
    void onParamChanged(const QString&);
    void onNuclideChanged(const QString&);
    void onXAxisChanged(int);

private:
    void buildUi();
    void refreshCombos();
    void refreshData();

    DepBridge        bridge_;
    ParamTableModel* tableModel_ = nullptr;

    // ВАЖЛИВО: саме * (вказівники), не об’єкти
    QComboBox *materialBox_ = nullptr;
    QComboBox *paramBox_    = nullptr;
    QComboBox *nuclideBox_  = nullptr;
    QComboBox *xAxisBox_    = nullptr;
    QComboBox *yScaleBox_   = nullptr; // якщо додавали лог-вісь

    QTableView* table_      = nullptr;
    QChartView* chartView_  = nullptr;

    QAction* openAct_       = nullptr;
    QAction* exportCsvAct_  = nullptr;

    enum class XAxis { BU, DAYS } xAxis_ = XAxis::BU;
    enum class YScale { Linear, Log10 } yScale_ = YScale::Linear; // якщо додавали лог-вісь
};
