#include "GeometryCheckDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include <QTime>
#include <QBrush>

// OpenCASCADE
#include <TopoDS_Shape.hxx>

GeometryCheckDialog::GeometryCheckDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("几何检查"));
    resize(900, 600);
    setupUI();
}

GeometryCheckDialog::~GeometryCheckDialog() = default;

void GeometryCheckDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // 检查选项区域
    m_checkOptionsGroup = new QGroupBox(QString::fromUtf8("检查类型"), this);
    auto* optionsLayout = new QVBoxLayout(m_checkOptionsGroup);

    m_cbDuplicateFaces = new QCheckBox(QString::fromUtf8("重复面：存在重复面"), this);
    m_cbInvalidFaces = new QCheckBox(QString::fromUtf8("错误面：存在错误面"), this);
    m_cbSmallFaces = new QCheckBox(QString::fromUtf8("微小面：存在微小面"), this);
    m_cbSmallEdges = new QCheckBox(QString::fromUtf8("微小边：存在微小边"), this);

    // 默认全部选中
    m_cbDuplicateFaces->setChecked(true);
    m_cbInvalidFaces->setChecked(true);
    m_cbSmallFaces->setChecked(true);
    m_cbSmallEdges->setChecked(true);

    optionsLayout->addWidget(m_cbDuplicateFaces);
    optionsLayout->addWidget(m_cbInvalidFaces);
    optionsLayout->addWidget(m_cbSmallFaces);
    optionsLayout->addWidget(m_cbSmallEdges);

    // 全选/取消按钮
    auto* buttonLayout = new QHBoxLayout();
    auto* btnCheckAll = new QPushButton(QString::fromUtf8("全选"), this);
    auto* btnUncheckAll = new QPushButton(QString::fromUtf8("取消"), this);
    buttonLayout->addWidget(btnCheckAll);
    buttonLayout->addWidget(btnUncheckAll);
    optionsLayout->addLayout(buttonLayout);

    connect(btnCheckAll, &QPushButton::clicked, this, &GeometryCheckDialog::onCheckAll);
    connect(btnUncheckAll, &QPushButton::clicked, this, &GeometryCheckDialog::onUncheckAll);

    mainLayout->addWidget(m_checkOptionsGroup);

    // 参数设置
    auto* paramGroup = new QGroupBox(QString::fromUtf8("检查参数"), this);
    auto* paramLayout = new QHBoxLayout(paramGroup);
    paramLayout->addWidget(new QLabel(QString::fromUtf8("公差:"), this));
    m_toleranceSpin = new QDoubleSpinBox(this);
    m_toleranceSpin->setRange(1e-6, 10.0);
    m_toleranceSpin->setValue(0.01);
    m_toleranceSpin->setDecimals(6);
    paramLayout->addWidget(m_toleranceSpin);
    paramLayout->addStretch();
    mainLayout->addWidget(paramGroup);

    // 执行按钮
    auto* btnRun = new QPushButton(QString::fromUtf8("🔍 执行检查"), this);
    btnRun->setStyleSheet(QString::fromUtf8("background-color:#4CAF50;color:white;font-weight:bold;padding:10px;font-size:14px;"));
    connect(btnRun, &QPushButton::clicked, this, &GeometryCheckDialog::onRun);
    mainLayout->addWidget(btnRun);

    // 结果区域
    auto* resultLayout = new QHBoxLayout();
    resultLayout->addWidget(new QLabel(QString::fromUtf8("检查结果:"), this));

    // 问题计数标签
    m_problemCountLabel = new QLabel(QString::fromUtf8("问题数量: 0"), this);
    m_problemCountLabel->setStyleSheet(QString::fromUtf8("font-weight:bold;color:red;"));
    resultLayout->addWidget(m_problemCountLabel);
    resultLayout->addStretch();

    // 导出报告按钮
    auto* btnExport = new QPushButton(QString::fromUtf8("导出报告"), this);
    connect(btnExport, &QPushButton::clicked, this, &GeometryCheckDialog::onExportReport);
    resultLayout->addWidget(btnExport);

    mainLayout->addLayout(resultLayout);

    // 结果树
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels(QStringList() 
        << QString::fromUtf8("类型") 
        << QString::fromUtf8("索引") 
        << QString::fromUtf8("描述"));
    m_resultsTree->setColumnWidth(0, 120);
    m_resultsTree->setColumnWidth(1, 80);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setRootIsDecorated(false);
    mainLayout->addWidget(m_resultsTree);
}

void GeometryCheckDialog::setShape(const TopoDS_Shape& shape)
{
    m_currentShape = shape;
    m_resultsTree->clear();
    m_problemCount = 0;
}

void GeometryCheckDialog::onCheckAll()
{
    m_cbDuplicateFaces->setChecked(true);
    m_cbInvalidFaces->setChecked(true);
    m_cbSmallFaces->setChecked(true);
    m_cbSmallEdges->setChecked(true);
}

void GeometryCheckDialog::onUncheckAll()
{
    m_cbDuplicateFaces->setChecked(false);
    m_cbInvalidFaces->setChecked(false);
    m_cbSmallFaces->setChecked(false);
    m_cbSmallEdges->setChecked(false);
}

void GeometryCheckDialog::onRun()
{
    if (!m_checker) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("检查器未初始化"));
        return;
    }

    if (m_currentShape.IsNull()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载几何体"));
        return;
    }

    updateCheckOptions();

    m_resultsTree->clear();
    m_problemCount = 0;

    // 执行检查
    if (m_checker->checkShape(m_currentShape)) {
        updateResultsTree();
        QMessageBox::information(this, QString::fromUtf8("完成"), 
            QString::fromUtf8("检查完成！共发现 %1 个问题").arg(m_problemCount));
        emit checkingFinished(m_problemCount);
    }
}

void GeometryCheckDialog::onExportReport()
{
    if (m_resultsTree->topLevelItemCount() == 0) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("没有检查结果可导出"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("导出检查报告"), 
        QString("geometry_check_report_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        QString::fromUtf8("文本文件 (*.txt)"));

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法创建文件"));
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // 写入报告头
    out << QString::fromUtf8("========================================\n");
    out << QString::fromUtf8("      几何检查报告\n");
    out << QString::fromUtf8("========================================\n\n");
    out << QString::fromUtf8("生成时间: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    out << QString::fromUtf8("问题数量: %1\n\n").arg(m_problemCount);
    out << QString::fromUtf8("----------------------------------------\n");
    out << QString::fromUtf8("检查结果详情:\n");
    out << QString::fromUtf8("----------------------------------------\n\n");

    // 写入检查项
    for (int i = 0; i < m_resultsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_resultsTree->topLevelItem(i);
        out << QString::fromUtf8("[%1] 类型: %2\n")
            .arg(i + 1).arg(item->text(0));
        out << QString::fromUtf8("    索引: %1\n").arg(item->text(1));
        out << QString::fromUtf8("    描述: %1\n\n").arg(item->text(2));
    }

    out << QString::fromUtf8("----------------------------------------\n");
    out << QString::fromUtf8("报告结束\n");
    out << QString::fromUtf8("========================================\n");

    file.close();
    QMessageBox::information(this, QString::fromUtf8("成功"), QString::fromUtf8("报告已导出: %1").arg(fileName));
}

void GeometryCheckDialog::updateCheckOptions()
{
    if (!m_checker) return;

    GeomChecker::CheckOptions options = GeomChecker::CheckNone;

    if (m_cbDuplicateFaces->isChecked())
        options |= GeomChecker::CheckDuplicateFaces;
    if (m_cbInvalidFaces->isChecked())
        options |= GeomChecker::CheckInvalidFaces;
    if (m_cbSmallFaces->isChecked())
        options |= GeomChecker::CheckSmallFaces;
    if (m_cbSmallEdges->isChecked())
        options |= GeomChecker::CheckSmallEdges;

    m_checker->setCheckOptions(options);
    m_checker->setTolerance(m_toleranceSpin->value());
}

void GeometryCheckDialog::updateResultsTree()
{
    m_resultsTree->clear();
    m_problemCount = 0;

    const QList<CheckResultItem>& results = m_checker->getResults();
    for (const auto& result : results) {
        auto* item = new QTreeWidgetItem(m_resultsTree);
        item->setText(0, result.getTypeString());
        item->setText(1, QString::number(result.index));
        item->setText(2, result.getDisplayString());

        // 设置颜色
        switch (result.type) {
            case CheckResultItem::InvalidFace:
            case CheckResultItem::SelfIntersectingFace:
                item->setForeground(0, QBrush(Qt::red));
                item->setForeground(2, QBrush(Qt::red));
                break;
            case CheckResultItem::SmallFace:
            case CheckResultItem::SmallEdge:
                item->setForeground(0, QBrush(QColor(255, 165, 0)));
                break;
            default:
                item->setForeground(0, QBrush(Qt::darkBlue));
                break;
        }

        m_problemCount++;
    }

    m_resultsTree->resizeColumnToContents(0);
    m_resultsTree->resizeColumnToContents(1);

    // 更新问题数量标签
    if (m_problemCountLabel) {
        m_problemCountLabel->setText(QString::fromUtf8("问题数量: %1").arg(m_problemCount));
    }
}

bool GeometryCheckDialog::runChecks()
{
    if (!m_checker || m_currentShape.IsNull()) {
        return false;
    }

    updateCheckOptions();
    m_resultsTree->clear();
    m_problemCount = 0;

    return m_checker->checkShape(m_currentShape);
}