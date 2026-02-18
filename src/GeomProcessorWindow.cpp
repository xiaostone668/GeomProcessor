#include "GeomProcessorWindow.h"

#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QApplication>
#include <QStatusBar>
#include <QStandardPaths>
#include <QDir>

// OCC
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>
#include <TopoDS_Shape.hxx>

GeomProcessorWindow::GeomProcessorWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUI();
    initOCC();

    m_processor = new GeomProcessor(this);
    m_receiver  = new GeomReceiver(this);

    // Processor signals
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::displayCurrentShape);
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::refreshFaceList);
    connect(m_processor, &GeomProcessor::shapeChanged,
            this, &GeomProcessorWindow::updateStatusInfo);
    connect(m_processor, &GeomProcessor::progressUpdated,
            m_progressBar, &QProgressBar::setValue);
    connect(m_processor, &GeomProcessor::errorOccurred, this, [this](const QString& e){
        m_statusLabel->setText("错误: " + e);
        QMessageBox::warning(this, "几何处理错误", e);
    });

    // Receiver signals
    connect(m_receiver, &GeomReceiver::geometryReceived,
            this, &GeomProcessorWindow::onGeometryReceived);
    connect(m_receiver, &GeomReceiver::statusChanged, m_ipcLabel, &QLabel::setText);
    connect(m_receiver, &GeomReceiver::errorOccurred, this, [this](const QString& e){
        m_ipcLabel->setText("IPC 错误: " + e);
    });

    m_receiver->start();

    setWindowTitle("GeomProcessor - 几何数据处理器");
    resize(1280, 800);
}

GeomProcessorWindow::~GeomProcessorWindow()
{
    if (m_receiver) m_receiver->stop();
}

void GeomProcessorWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

// ---------------------------------------------------------------------------
// UI Setup
// ---------------------------------------------------------------------------

void GeomProcessorWindow::setupUI()
{
    setupMenuBar();
    setupToolBar();

    // Central: OCC view (created before initOCC so the widget exists)
    m_occWidget = new OccViewWidget(this);
    setCentralWidget(m_occWidget);

    setupOperationPanel();
    setupFaceList();
    setupStatusBar();
}

void GeomProcessorWindow::setupMenuBar()
{
    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    auto* loadAct  = new QAction(tr("打开 STEP 文件..."), this);
    loadAct->setShortcut(QKeySequence::Open);
    connect(loadAct, &QAction::triggered, this, &GeomProcessorWindow::onLoadSTEP);
    fileMenu->addAction(loadAct);

    auto* saveAct = new QAction(tr("保存 STEP 文件..."), this);
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &GeomProcessorWindow::onSaveSTEP);
    fileMenu->addAction(saveAct);

    fileMenu->addSeparator();
    auto* exitAct = new QAction(tr("退出"), this);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(exitAct);

    auto* opMenu = menuBar()->addMenu(tr("操作(&O)"));
    auto* stitchAct = new QAction(tr("缝合 Shell"), this);
    connect(stitchAct, &QAction::triggered, this, &GeomProcessorWindow::onStitchShells);
    opMenu->addAction(stitchAct);

    auto* healAct = new QAction(tr("修复形状"), this);
    connect(healAct, &QAction::triggered, this, &GeomProcessorWindow::onHealShape);
    opMenu->addAction(healAct);

    auto* delAct = new QAction(tr("删除选中面"), this);
    connect(delAct, &QAction::triggered, this, &GeomProcessorWindow::onDeleteSelectedFaces);
    opMenu->addAction(delAct);

    auto* sendAct = new QAction(tr("发送结果到 SimulationTool"), this);
    connect(sendAct, &QAction::triggered, this, &GeomProcessorWindow::onSendResultBack);
    opMenu->addAction(sendAct);
}

void GeomProcessorWindow::setupToolBar()
{
    auto* tb = addToolBar(tr("操作"));

    auto* loadAct   = tb->addAction(tr("📂 打开"));
    connect(loadAct, &QAction::triggered, this, &GeomProcessorWindow::onLoadSTEP);

    auto* saveAct   = tb->addAction(tr("💾 保存"));
    connect(saveAct, &QAction::triggered, this, &GeomProcessorWindow::onSaveSTEP);

    tb->addSeparator();

    auto* stitchAct = tb->addAction(tr("🔗 缝合"));
    connect(stitchAct, &QAction::triggered, this, &GeomProcessorWindow::onStitchShells);

    auto* healAct   = tb->addAction(tr("🔧 修复"));
    connect(healAct, &QAction::triggered, this, &GeomProcessorWindow::onHealShape);

    auto* delAct    = tb->addAction(tr("✂ 删除面"));
    connect(delAct, &QAction::triggered, this, &GeomProcessorWindow::onDeleteSelectedFaces);

    auto* offsetAct = tb->addAction(tr("📐 偏移"));
    connect(offsetAct, &QAction::triggered, this, &GeomProcessorWindow::onOffsetShape);

    tb->addSeparator();

    auto* sendAct   = tb->addAction(tr("📤 发回"));
    connect(sendAct, &QAction::triggered, this, &GeomProcessorWindow::onSendResultBack);
}

void GeomProcessorWindow::setupOperationPanel()
{
    m_opDock = new QDockWidget(tr("操作参数"), this);

    auto* w   = new QWidget(m_opDock);
    auto* lay = new QVBoxLayout(w);

    // Stitch
    auto* stitchBox = new QGroupBox(tr("缝合 (Stitch)"));
    auto* sf = new QFormLayout(stitchBox);
    m_stitchTolSpin = new QDoubleSpinBox();
    m_stitchTolSpin->setRange(1e-6, 10.0);
    m_stitchTolSpin->setValue(1e-3);
    m_stitchTolSpin->setDecimals(6);
    sf->addRow(tr("公差:"), m_stitchTolSpin);
    auto* stitchBtn = new QPushButton(tr("执行缝合"));
    connect(stitchBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onStitchShells);
    sf->addRow(stitchBtn);
    lay->addWidget(stitchBox);

    // Heal
    auto* healBox = new QGroupBox(tr("形状修复 (Heal)"));
    auto* hf = new QFormLayout(healBox);
    m_healPrecSpin = new QDoubleSpinBox();
    m_healPrecSpin->setRange(1e-7, 1.0);
    m_healPrecSpin->setValue(1e-4);
    m_healPrecSpin->setDecimals(7);
    hf->addRow(tr("精度:"), m_healPrecSpin);
    auto* healBtn = new QPushButton(tr("执行修复"));
    connect(healBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onHealShape);
    hf->addRow(healBtn);
    lay->addWidget(healBox);

    // Offset
    auto* offsetBox = new QGroupBox(tr("偏移 (Offset)"));
    auto* of = new QFormLayout(offsetBox);
    m_offsetSpin = new QDoubleSpinBox();
    m_offsetSpin->setRange(-100.0, 100.0);
    m_offsetSpin->setValue(1.0);
    m_offsetSpin->setDecimals(3);
    of->addRow(tr("偏移量:"), m_offsetSpin);
    auto* offsetBtn = new QPushButton(tr("执行偏移"));
    connect(offsetBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onOffsetShape);
    of->addRow(offsetBtn);
    lay->addWidget(offsetBox);

    // Send back
    auto* sendBtn = new QPushButton(tr("📤 发送结果到 SimulationTool"));
    sendBtn->setStyleSheet("background-color:#2196F3;color:white;font-weight:bold;");
    connect(sendBtn, &QPushButton::clicked, this, &GeomProcessorWindow::onSendResultBack);
    lay->addWidget(sendBtn);

    lay->addStretch();
    m_opDock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, m_opDock);
}

void GeomProcessorWindow::setupFaceList()
{
    m_faceDock = new QDockWidget(tr("面列表 (选中后可删除)"), this);
    m_faceList = new QListWidget(m_faceDock);
    m_faceList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_faceList, &QListWidget::itemSelectionChanged,
            this, &GeomProcessorWindow::onFaceListSelectionChanged);
    m_faceDock->setWidget(m_faceList);
    addDockWidget(Qt::LeftDockWidgetArea, m_faceDock);
}

void GeomProcessorWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("就绪"));
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setMinimumWidth(150);

    m_ipcLabel = new QLabel(tr("IPC: 未连接"));
    m_ipcLabel->setMinimumWidth(300);

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addWidget(m_progressBar);
    statusBar()->addPermanentWidget(m_ipcLabel);
}

// ---------------------------------------------------------------------------
// OCC
// ---------------------------------------------------------------------------

void GeomProcessorWindow::initOCC()
{
    try {
        m_displayConnection = new Aspect_DisplayConnection();
        m_graphicDriver     = new OpenGl_GraphicDriver(m_displayConnection);

        m_viewer = new V3d_Viewer(m_graphicDriver);
        m_viewer->SetDefaultLights();
        m_viewer->SetLightOn();

        m_context = new AIS_InteractiveContext(m_viewer);

        if (m_occWidget) {
            m_occWidget->init(m_viewer, m_context);
        }
    }
    catch (const Standard_Failure& e) {
        QMessageBox::critical(this, tr("OCC 初始化失败"),
            QString::fromLatin1(e.GetMessageString()));
    }
}

// ---------------------------------------------------------------------------
// IPC Slot
// ---------------------------------------------------------------------------

void GeomProcessorWindow::onGeometryReceived(const QString& stepFile, uint32_t seqNo)
{
    m_currentSeq   = seqNo;
    m_lastGeomFile = stepFile;

    m_statusLabel->setText(QString("正在加载几何 #%1 ...").arg(seqNo));
    m_progressBar->setValue(0);

    if (m_processor->loadSTEP(stepFile)) {
        m_statusLabel->setText(
            QString("已加载几何 #%1 | %2 个面 | %3 个实体")
            .arg(seqNo).arg(m_processor->numFaces()).arg(m_processor->numSolids()));
    } else {
        m_statusLabel->setText("加载失败: " + m_processor->getLastError());
        m_receiver->sendError(m_processor->getLastError());
    }
}

// ---------------------------------------------------------------------------
// Operation Slots
// ---------------------------------------------------------------------------

void GeomProcessorWindow::onStitchShells()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "请先加载 STEP 文件或等待 SimulationTool 发送几何");
        return;
    }
    m_statusLabel->setText("正在缝合...");
    m_progressBar->setValue(0);
    if (m_processor->stitchShells(m_stitchTolSpin->value())) {
        m_statusLabel->setText(QString("缝合完成 | %1 个面 | %2 个实体")
            .arg(m_processor->numFaces()).arg(m_processor->numSolids()));
    }
}

void GeomProcessorWindow::onDeleteSelectedFaces()
{
    if (!m_processor->hasShape()) return;

    std::vector<int> selected;
    for (auto* item : m_faceList->selectedItems()) {
        selected.push_back(item->data(Qt::UserRole).toInt());
    }
    if (selected.empty()) {
        QMessageBox::information(this, "提示", "请在左侧面列表中选择要删除的面");
        return;
    }

    m_statusLabel->setText(QString("正在删除 %1 个面...").arg(selected.size()));
    m_progressBar->setValue(0);
    if (m_processor->deleteFaces(selected)) {
        m_statusLabel->setText(QString("删除完成 | 剩余 %1 个面").arg(m_processor->numFaces()));
    }
}

void GeomProcessorWindow::onHealShape()
{
    if (!m_processor->hasShape()) return;
    m_statusLabel->setText("正在修复形状...");
    m_progressBar->setValue(0);
    if (m_processor->healShape(m_healPrecSpin->value())) {
        m_statusLabel->setText("修复完成");
    }
}

void GeomProcessorWindow::onOffsetShape()
{
    if (!m_processor->hasShape()) return;
    m_statusLabel->setText("正在偏移...");
    m_progressBar->setValue(0);
    if (m_processor->offsetShape(m_offsetSpin->value())) {
        m_statusLabel->setText("偏移完成");
    }
}

void GeomProcessorWindow::onLoadSTEP()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("打开 STEP 文件"), QString(),
        tr("STEP Files (*.step *.stp);;All Files (*.*)"));
    if (path.isEmpty()) return;
    m_statusLabel->setText("正在加载...");
    if (!m_processor->loadSTEP(path)) {
        QMessageBox::warning(this, "加载失败", m_processor->getLastError());
    }
}

void GeomProcessorWindow::onSaveSTEP()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "没有几何体可保存");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this,
        tr("保存 STEP 文件"), QString(),
        tr("STEP Files (*.step *.stp);;All Files (*.*)"));
    if (path.isEmpty()) return;
    if (m_processor->saveSTEP(path)) {
        m_statusLabel->setText("已保存: " + path);
    } else {
        QMessageBox::warning(this, "保存失败", m_processor->getLastError());
    }
}

void GeomProcessorWindow::onSendResultBack()
{
    if (!m_processor->hasShape()) {
        QMessageBox::information(this, "提示", "没有几何体可发送");
        return;
    }

    // Write to temp file
    QString tmpDir  = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tmpFile = QDir(tmpDir).filePath("geomproc_result.stp");

    if (!m_processor->saveSTEP(tmpFile)) {
        QMessageBox::warning(this, "错误", "保存临时 STEP 文件失败: " + m_processor->getLastError());
        return;
    }

    if (m_receiver->sendResult(tmpFile)) {
        m_statusLabel->setText("结果已发送至 SimulationTool: " + tmpFile);
    } else {
        QMessageBox::warning(this, "IPC 错误", "发送失败，SimulationTool 可能未运行");
    }
}

void GeomProcessorWindow::onFaceListSelectionChanged()
{
    int n = m_faceList->selectedItems().size();
    if (n > 0) {
        m_statusLabel->setText(QString("已选择 %1 个面（点击工具栏[删除面]执行）").arg(n));
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void GeomProcessorWindow::refreshFaceList()
{
    m_faceList->clear();
    if (!m_processor->hasShape()) return;

    int idx = 0;
    TopExp_Explorer exp(m_processor->getShape(), TopAbs_FACE);
    for (; exp.More(); exp.Next(), ++idx) {
        auto* item = new QListWidgetItem(QString("面 %1").arg(idx));
        item->setData(Qt::UserRole, idx);
        m_faceList->addItem(item);
    }
}

void GeomProcessorWindow::displayCurrentShape()
{
    if (!m_context.IsNull()) {
        m_processor->displayShape(m_context, true);
    }
}

void GeomProcessorWindow::updateStatusInfo()
{
    m_statusLabel->setText(
        QString("几何就绪 | %1 个面 | %2 个实体")
        .arg(m_processor->numFaces())
        .arg(m_processor->numSolids()));
}
