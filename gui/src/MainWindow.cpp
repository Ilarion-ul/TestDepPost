#include "MainWindow.h"
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>

#include <QtCharts/QChart>        // Додали!
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>

#include "CsvWriter.h"
#include "Logger.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
}


void MainWindow::buildUi() {
    auto* tb = addToolBar("Main");
    openAct_      = tb->addAction("Open dep.m");
    exportCsvAct_ = tb->addAction("Export CSV");
    connect(openAct_,      &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(exportCsvAct_, &QAction::triggered, this, &MainWindow::onExportCsv);

    // створення саме через new ... (бо поля — вказівники)
    materialBox_ = new QComboBox(this);
    paramBox_    = new QComboBox(this);
    nuclideBox_  = new QComboBox(this);
    xAxisBox_    = new QComboBox(this);
    xAxisBox_->addItems({"BU","DAYS"});

    // (якщо робили лог-шкалу Y)
    yScaleBox_ = new QComboBox(this);
    yScaleBox_->addItems({"Y: Linear","Y: Log10"});

    tb->addSeparator();
    tb->addWidget(materialBox_);
    tb->addWidget(paramBox_);
    tb->addWidget(nuclideBox_);
    tb->addSeparator();
    tb->addWidget(xAxisBox_);
    if (yScaleBox_) tb->addWidget(yScaleBox_);

    // НОВІ конекшени, без onSelectionChanged
    connect(materialBox_, &QComboBox::currentTextChanged, this, &MainWindow::onMaterialChanged);
    connect(paramBox_,    &QComboBox::currentTextChanged, this, &MainWindow::onParamChanged);
    connect(nuclideBox_,  &QComboBox::currentTextChanged, this, &MainWindow::onNuclideChanged);
    connect(xAxisBox_, &QComboBox::currentIndexChanged, this, &MainWindow::onXAxisChanged);
    if (yScaleBox_) {
        connect(yScaleBox_, &QComboBox::currentIndexChanged, this, [this](int i){
            yScale_ = (i==0) ? YScale::Linear : YScale::Log10;
            refreshData();
        });
    }

    table_ = new QTableView(this);
    tableModel_ = new ParamTableModel(this);
    table_->setModel(tableModel_);

    // БЕЗ QtCharts:: — типи з інклюдів вище
    chartView_ = new QChartView(new QChart(), this);
    chartView_->setRenderHint(QPainter::Antialiasing);

    auto* splitter = new QSplitter(this);
    splitter->addWidget(table_);
    splitter->addWidget(chartView_);
    splitter->setStretchFactor(0,1);
    splitter->setStretchFactor(1,1);
    setCentralWidget(splitter);

    resize(1200, 700);
    setWindowTitle("SerpentPost GUI");
}



void MainWindow::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, "Open dep.m", {}, "dep.m files (*.m *.*)");
    if (path.isEmpty()) return;
    QString err;
    if (!bridge_.loadFile(path, err)) {
        QMessageBox::warning(this, "Error", "Failed to load dep.m:\n"+err);
        return;
    }
    slog::Logger::info("GUI: file opened: {}", path.toStdString());
    refreshCombos();
    refreshData();
}

void MainWindow::onExportCsv() {
    if (!bridge_.hasData()) { QMessageBox::information(this, "Export", "Load dep.m first."); return; }
    const QString dir = QFileDialog::getExistingDirectory(this, "Select output folder");
    if (dir.isEmpty()) return;
    try {
        CsvWriter::writePerMaterial(reinterpret_cast<const DepData&>(*(const DepData*)&bridge_), dir.toStdString());
        // ↑ трюк не потрібен, якщо додати метод доступу; краще ось так:
        // CsvWriter::writePerMaterial(bridge_.data(), dir.toStdString());
        // але тоді зроби у DepBridge геттер на DepData.
        QMessageBox::information(this, "Export", "CSV exported.");
        slog::Logger::info("CSV exported to {}", dir.toStdString());
    } catch (const std::exception& ex) {
        slog::Logger::error("CSV export failed: {}", ex.what());
        QMessageBox::warning(this, "Export error", ex.what());
    }
}

void MainWindow::onMaterialChanged(const QString&) {
    paramBox_->blockSignals(true);
    paramBox_->clear();
    if (materialBox_->count() > 0)
        paramBox_->addItems(bridge_.parameters(materialBox_->currentText()));
    paramBox_->blockSignals(false);
    refreshData();
}
void MainWindow::onParamChanged(const QString&)   { refreshData(); }
void MainWindow::onNuclideChanged(const QString&){ refreshData(); }
//void MainWindow::onXAxisChanged(int)              { refreshData(); }

void MainWindow::onXAxisChanged(int idx) {
    xAxis_ = (idx==0) ? XAxis::BU : XAxis::DAYS;
    refreshData();
}

void MainWindow::refreshCombos() {
    materialBox_->blockSignals(true);
    paramBox_->blockSignals(true);
    nuclideBox_->blockSignals(true);

    materialBox_->clear();
    paramBox_->clear();
    nuclideBox_->clear();

    materialBox_->addItems(bridge_.materials());
    if (materialBox_->count()>0)
        paramBox_->addItems(bridge_.parameters(materialBox_->currentText()));
    nuclideBox_->addItems(bridge_.nuclides());
    if (nuclideBox_->findText("Total")>=0) nuclideBox_->setCurrentText("Total");

    materialBox_->blockSignals(false);
    paramBox_->blockSignals(false);
    nuclideBox_->blockSignals(false);
}

void MainWindow::refreshData() {
    if (!bridge_.hasData()) return;
    const auto mat = materialBox_->currentText();
    const auto par = paramBox_->currentText();
    const auto iso = nuclideBox_->currentText();
    if (mat.isEmpty() || par.isEmpty() || iso.isEmpty()) return;

    auto s = bridge_.series(mat, par, iso);
    tableModel_->setSeries(s);

    auto* chart = new QChart();
    auto* line  = new QLineSeries();
    double xmin = std::numeric_limits<double>::infinity();
    double xmax = -std::numeric_limits<double>::infinity();
    double ymin = std::numeric_limits<double>::infinity();
    double ymax = -std::numeric_limits<double>::infinity();

    for (const auto& p : s) {
        const double x = (xAxis_==XAxis::BU ? p.bu : p.days);
        line->append(x, p.value);
        xmin = std::min(xmin, x); xmax = std::max(xmax, x);
        ymin = std::min(ymin, p.value); ymax = std::max(ymax, p.value);
    }
    chart->addSeries(line);

    // X-вісь (лінійна)
    auto* axX = new QValueAxis();
    axX->setTitleText(xAxis_==XAxis::BU ? "BU" : "DAYS");
    if (std::isfinite(xmin) && std::isfinite(xmax) && xmin < xmax) {
        const double pad = (xmax - xmin) * 0.05;
        axX->setRange(xmin - pad, xmax + pad);
    }
    chart->addAxis(axX, Qt::AlignBottom);
    line->attachAxis(axX);

    // Y-вісь (Linear або Log10)
    #include <QtCharts/QLogValueAxis>

// ... після додавання серії та X-осі:

    QAbstractAxis* axY = nullptr;
    if (yScale_ == YScale::Log10) {
        // зберемо тільки додатні точки
        QList<QPointF> filtered;
        filtered.reserve((int)s.size());
        double minPos = std::numeric_limits<double>::infinity();
        double maxPos = 0.0;

        for (const auto& p : s) {
            if (p.value > 0.0) {
                const double x = (xAxis_==XAxis::BU ? p.bu : p.days);
                filtered.append(QPointF(x, p.value));
                minPos = std::min(minPos, p.value);
                maxPos = std::max(maxPos, p.value);
            }
        }

        // якщо все ≤0 — просто показуємо порожній графік із повідомленням
        line->replace(filtered);

        auto* logY = new QLogValueAxis();
        logY->setTitleText("Value (log10)");
        if (std::isfinite(minPos) && maxPos > 0.0) {
            // невеликий відступ зверху/знизу
            const double lo = std::max(minPos * 0.8, 1e-300);
            const double hi = std::max(maxPos * 1.25, lo * 10.0);
            logY->setRange(lo, hi);
        } else {
            // fallback, якщо всі нулі/від’ємні
            logY->setRange(1.0, 10.0);
        }
        axY = logY;
    } else {
        auto* linY = new QValueAxis();
        linY->setTitleText("Value");
        linY->setLabelFormat("%.3e"); // зручне відображення малих чисел
        if (std::isfinite(ymin) && std::isfinite(ymax) && ymin < ymax) {
            const double pad = (ymax - ymin) * 0.1 + (ymax==ymin ? 1.0 : 0.0);
            linY->setRange(ymin - pad, ymax + pad);
        }
        axY = linY;
    }
    chart->addAxis(axY, Qt::AlignLeft);
    line->attachAxis(axY);


    chart->setTitle(QString("%1 / %2 / %3").arg(mat, par, iso));
    chartView_->setChart(chart);
}