#include "ParasolidCheckRepairWidget.h"
#include "ParasolidChecker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QLineEdit>

ParasolidCheckRepairWidget::ParasolidCheckRepairWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

ParasolidCheckRepairWidget::~ParasolidCheckRepairWidget() = default;

void ParasolidCheckRepairWidget::setChecker(ParasolidChecker* checker)
{
    m_checker = checker;
}

void ParasolidCheckRepairWidget::setShapeData(const QVariant& parasolidData)
{
    m_parasolidData = parasolidData;
    m_resultsTree->clear();
    m_btnRepair->setEnabled(false);
}

bool ParasolidCheckRepairWidget::runChecks()
{
    if (!m_checker) {
        QMessageBox::information(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("检查器未初始化，请重新启动程序"));
        return false;
    }

    if (m_parasolidData.isNull()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("请先加载Parasolid几何数据"));
        return false;
    }

    m_resultsTree->clear();

    updateCheckOptions();

    if (m_checker->checkShape(m_parasolidData)) {
        updateResultsTree();
        
        int numProblems = m_checker->getProblemCount();
        emit checkingFinished(numProblems);
        return true;
    }

    return false;
}

void ParasolidCheckRepairWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    // 顶部标题
    auto* titleLabel = new QLabel(QString::fromUtf8("检查"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 检查对象设置
    auto* objectGroup = new QGroupBox(QString::fromUtf8("检查对象"), this);
    auto* objectLayout = new QVBoxLayout(objectGroup);
    objectLayout->setSpacing(6);

    // 文字提示输入框
    m_comboSelection = new QLineEdit(this);
    m_comboSelection->setPlaceholderText(QString::fromUtf8("输入检查对象名称..."));
    objectLayout->addWidget(m_comboSelection);

    // 距离公差
    auto* distLayout = new QHBoxLayout();
    auto* distLabel = new QLabel(QString::fromUtf8("距离公差："), this);
    m_spinDistancePrecision = new QDoubleSpinBox(this);
    m_spinDistancePrecision->setRange(0.001, 100.0);
    m_spinDistancePrecision->setValue(0.1);
    m_spinDistancePrecision->setSingleStep(0.1);
    m_spinDistancePrecision->setDecimals(3);
    m_spinDistancePrecision->setMaximumWidth(80);
    auto* distUnit = new QLabel(QString::fromUtf8("mm"), this);
    distLayout->addStretch();
    distLayout->addWidget(distLabel);
    distLayout->addWidget(m_spinDistancePrecision);
    distLayout->addWidget(distUnit);
    objectLayout->addLayout(distLayout);

    // 角度公差
    auto* angleLayout = new QHBoxLayout();
    auto* angleLabel = new QLabel(QString::fromUtf8("角度公差："), this);
    m_spinAnglePrecision = new QDoubleSpinBox(this);
    m_spinAnglePrecision->setRange(0.01, 180.0);
    m_spinAnglePrecision->setValue(0.1);
    m_spinAnglePrecision->setSingleStep(0.1);
    m_spinAnglePrecision->setDecimals(2);
    m_spinAnglePrecision->setMaximumWidth(80);
    auto* angleUnit = new QLabel(QString::fromUtf8("°"), this);
    angleLayout->addStretch();
    angleLayout->addWidget(angleLabel);
    angleLayout->addWidget(m_spinAnglePrecision);
    angleLayout->addWidget(angleUnit);
    objectLayout->addLayout(angleLayout);

    mainLayout->addWidget(objectGroup);

    // 检查项区域
    auto* checkItemGroup = new QGroupBox(QString::fromUtf8("检查项"), this);
    auto* checkItemLayout = new QVBoxLayout(checkItemGroup);
    checkItemLayout->setSpacing(6);

    // 面类型分组
    auto* faceGroup = new QGroupBox(QString::fromUtf8("面类型"), this);
    auto* faceLayout = new QVBoxLayout(faceGroup);
    faceLayout->setSpacing(4);

    m_cbTinyFace = new QCheckBox(QString::fromUtf8("微小面"), this);
    m_cbSliverFace = new QCheckBox(QString::fromUtf8("狭长面"), this);
    m_cbSpikeFace = new QCheckBox(QString::fromUtf8("尖刺面"), this);
    m_cbDuplicateFace = new QCheckBox(QString::fromUtf8("重复面"), this);
    m_cbIntersectingFace = new QCheckBox(QString::fromUtf8("相交面"), this);

    m_cbTinyFace->setChecked(true);
    m_cbSliverFace->setChecked(true);
    m_cbSpikeFace->setChecked(true);
    m_cbDuplicateFace->setChecked(true);
    m_cbIntersectingFace->setChecked(true);

    m_cbTinyFace->setProperty("checkOption", (int)GeomChecker::CheckSmallFaces);
    m_cbSliverFace->setProperty("checkOption", (int)GeomChecker::CheckSliverFaces);
    m_cbSpikeFace->setProperty("checkOption", (int)GeomChecker::CheckSpikes);
    m_cbDuplicateFace->setProperty("checkOption", (int)GeomChecker::CheckDuplicateFaces);
    m_cbIntersectingFace->setProperty("checkOption", (int)GeomChecker::CheckIntersectingFaces);

    faceLayout->addWidget(m_cbTinyFace);
    faceLayout->addWidget(m_cbSliverFace);
    faceLayout->addWidget(m_cbSpikeFace);
    faceLayout->addWidget(m_cbDuplicateFace);
    faceLayout->addWidget(m_cbIntersectingFace);

    checkItemLayout->addWidget(faceGroup);

    // 边类型分组
    auto* edgeGroup = new QGroupBox(QString::fromUtf8("边类型"), this);
    auto* edgeLayout = new QVBoxLayout(edgeGroup);
    edgeLayout->setSpacing(4);

    m_cbExtraPoint = new QCheckBox(QString::fromUtf8("冗余点"), this);
    m_cbExtraEdge = new QCheckBox(QString::fromUtf8("冗余边"), this);
    m_cbSmallEdge = new QCheckBox(QString::fromUtf8("微小边"), this);
    m_cbGap = new QCheckBox(QString::fromUtf8("间隙"), this);

    m_cbExtraPoint->setChecked(true);
    m_cbExtraEdge->setChecked(true);
    m_cbSmallEdge->setChecked(true);
    m_cbGap->setChecked(true);

    m_cbExtraPoint->setProperty("checkOption", (int)GeomChecker::CheckRedundantVertices);
    m_cbExtraEdge->setProperty("checkOption", (int)GeomChecker::CheckRedundantEdges);
    m_cbSmallEdge->setProperty("checkOption", (int)GeomChecker::CheckSmallEdges);
    m_cbGap->setProperty("checkOption", (int)GeomChecker::CheckGaps);

    edgeLayout->addWidget(m_cbExtraPoint);
    edgeLayout->addWidget(m_cbExtraEdge);
    edgeLayout->addWidget(m_cbSmallEdge);
    edgeLayout->addWidget(m_cbGap);

    checkItemLayout->addWidget(edgeGroup);

    mainLayout->addWidget(checkItemGroup);

    // 执行检查按钮
    m_btnCheck = new QPushButton(QString::fromUtf8("检查"), this);
    m_btnCheck->setStyleSheet(QString::fromUtf8(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 10px;"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    ));
    connect(m_btnCheck, &QPushButton::clicked, this, &ParasolidCheckRepairWidget::onRunCheck);
    mainLayout->addWidget(m_btnCheck);

    // 结果展示区
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels(QStringList() 
        << QString::fromUtf8("问题类型") 
        << QString::fromUtf8("描述") 
        << QString::fromUtf8("索引"));
    m_resultsTree->setColumnWidth(0, 80);
    m_resultsTree->setColumnWidth(1, 120);
    m_resultsTree->setColumnWidth(2, 50);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setRootIsDecorated(true);
    m_resultsTree->setMinimumHeight(200);
    
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked,
            this, &ParasolidCheckRepairWidget::onResultItemDoubleClicked);
    m_resultsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_resultsTree, &QTreeWidget::customContextMenuRequested,
            this, &ParasolidCheckRepairWidget::onResultItemContextMenu);
    
    mainLayout->addWidget(m_resultsTree);
    
    // 底部一键修复按钮
    m_btnRepair = new QPushButton(QString::fromUtf8("一键修复"), this);
    m_btnRepair->setStyleSheet(QString::fromUtf8(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 10px;"
        "  border-radius: 4px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    ));
    connect(m_btnRepair, &QPushButton::clicked, this, &ParasolidCheckRepairWidget::onRunRepair);
    mainLayout->addWidget(m_btnRepair);
    
    mainLayout->addStretch();
    
    // 初始状态：修复按钮禁用
    m_btnRepair->setEnabled(false);
}

void ParasolidCheckRepairWidget::onRunCheck()
{
    runChecks();
}

void ParasolidCheckRepairWidget::onRunRepair()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QString::fromUtf8("确认修复"),
        QString::fromUtf8("将一键自动修复错误问题，是否继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        QList<int> problemIndices;
        for (int i = 0; i < m_resultsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* categoryItem = m_resultsTree->topLevelItem(i);
            for (int j = 0; j < categoryItem->childCount(); ++j) {
                QTreeWidgetItem* item = categoryItem->child(j);
                if (!item->isDisabled()) {
                    item->text(2).toInt();
                }
            }
        }
        
        emit repairRequested(problemIndices);
        QMessageBox::information(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("Parasolid修复功能待实现"));
    }
}

void ParasolidCheckRepairWidget::onExpandAll()
{
    m_resultsTree->expandAll();
}

void ParasolidCheckRepairWidget::onCollapseAll()
{
    m_resultsTree->collapseAll();
}

void ParasolidCheckRepairWidget::onSelectFromCanvas()
{
    // 不再需要此功能
}

void ParasolidCheckRepairWidget::onSelectFromTree()
{
    // 不再需要此功能
}

void ParasolidCheckRepairWidget::onClearSelection()
{
    m_comboSelection->clear();
}

void ParasolidCheckRepairWidget::onCheckAll(int state)
{
    bool checked = (state == Qt::Checked);
    
    m_cbTinyFace->setChecked(checked);
    m_cbSliverFace->setChecked(checked);
    m_cbSpikeFace->setChecked(checked);
    m_cbDuplicateFace->setChecked(checked);
    m_cbIntersectingFace->setChecked(checked);
    
    m_cbExtraPoint->setChecked(checked);
    m_cbExtraEdge->setChecked(checked);
    m_cbSmallEdge->setChecked(checked);
    m_cbGap->setChecked(checked);
}

void ParasolidCheckRepairWidget::onUncheckAll()
{
    // 不再需要此功能
}

void ParasolidCheckRepairWidget::onResultItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    highlightResultItem(item);
}

void ParasolidCheckRepairWidget::onResultItemContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_resultsTree->itemAt(pos);
    if (!item || !item->parent()) {
        return;
    }
    
    QMenu contextMenu(this);
    QAction* excludeAction = contextMenu.addAction(QString::fromUtf8("排除此项"));
    
    QAction* action = contextMenu.exec(m_resultsTree->viewport()->mapToGlobal(pos));
    if (action == excludeAction) {
        onExcludeItem();
    }
}

void ParasolidCheckRepairWidget::onExcludeItem()
{
    QTreeWidgetItem* item = m_resultsTree->currentItem();
    if (!item || !item->parent()) {
        return;
    }
    
    item->setDisabled(true);
    item->setForeground(0, QBrush(Qt::gray));
    
    updateRepairButtonState();
}

void ParasolidCheckRepairWidget::updateCheckOptions()
{
    if (!m_checker) return;

    GeomChecker::CheckOptions options = GeomChecker::CheckNone;

    if (m_cbTinyFace->isChecked())
        options |= GeomChecker::CheckSmallFaces;
    if (m_cbSliverFace->isChecked())
        options |= GeomChecker::CheckSliverFaces;
    if (m_cbSpikeFace->isChecked())
        options |= GeomChecker::CheckSpikes;
    if (m_cbDuplicateFace->isChecked())
        options |= GeomChecker::CheckDuplicateFaces;
    if (m_cbIntersectingFace->isChecked())
        options |= GeomChecker::CheckIntersectingFaces;
    
    if (m_cbExtraPoint->isChecked())
        options |= GeomChecker::CheckRedundantVertices;
    if (m_cbExtraEdge->isChecked())
        options |= GeomChecker::CheckRedundantEdges;
    if (m_cbSmallEdge->isChecked())
        options |= GeomChecker::CheckSmallEdges;
    if (m_cbGap->isChecked())
        options |= GeomChecker::CheckGaps;

    m_checker->setCheckOptions(options);
}

void ParasolidCheckRepairWidget::updateResultsTree()
{
    m_resultsTree->clear();
    
    const QList<CheckResultItem>& results = m_checker->getResults();
    
    // 创建类别节点
    QTreeWidgetItem* faceCategory = new QTreeWidgetItem(m_resultsTree);
    faceCategory->setText(0, QString::fromUtf8("面类型"));
    
    QTreeWidgetItem* edgeCategory = new QTreeWidgetItem(m_resultsTree);
    edgeCategory->setText(0, QString::fromUtf8("边类型"));
    
    QTreeWidgetItem* gapCategory = new QTreeWidgetItem(m_resultsTree);
    gapCategory->setText(0, QString::fromUtf8("间隙"));
    
    int faceCount = 0;
    int edgeCount = 0;
    int gapCount = 0;
    
    for (const auto& result : results) {
        QTreeWidgetItem* categoryItem = nullptr;
        
        switch (result.type) {
            case CheckResultItem::SmallFace:
            case CheckResultItem::SliverFace:
            case CheckResultItem::Spike:
            case CheckResultItem::DuplicateFace:
            case CheckResultItem::IntersectingFace:
                categoryItem = faceCategory;
                break;
            case CheckResultItem::RedundantVertex:
            case CheckResultItem::RedundantEdge:
            case CheckResultItem::SmallEdge:
                categoryItem = edgeCategory;
                break;
            case CheckResultItem::Gap:
                categoryItem = gapCategory;
                break;
            case CheckResultItem::InvalidFace:
            case CheckResultItem::SelfIntersectingFace:
                categoryItem = faceCategory;
                break;
            default:
                continue;
        }
        
        if (categoryItem) {
            auto* item = new QTreeWidgetItem(categoryItem);
            item->setText(0, result.getTypeString());
            
            QString description;
            switch (result.type) {
                case CheckResultItem::SmallFace:
                    description = QString::fromUtf8("面%1：存在微小面").arg(result.index);
                    break;
                case CheckResultItem::DuplicateFace:
                    description = QString::fromUtf8("面组%1：存在重复面").arg(result.index);
                    break;
                case CheckResultItem::IntersectingFace:
                    description = QString::fromUtf8("面组%1：存在相交面").arg(result.index);
                    break;
                case CheckResultItem::RedundantVertex:
                    description = QString::fromUtf8("点%1：存在冗余点").arg(result.index);
                    break;
                case CheckResultItem::SmallEdge:
                    description = QString::fromUtf8("边%1：存在微小边").arg(result.index);
                    break;
                case CheckResultItem::Gap:
                    description = QString::fromUtf8("边组%1存在间隙").arg(result.index);
                    break;
                default:
                    description = result.getDisplayString();
            }
            item->setText(1, description);
            item->setText(2, QString::number(result.index));
            
            switch (result.type) {
                case CheckResultItem::InvalidFace:
                case CheckResultItem::SelfIntersectingFace:
                    item->setForeground(0, QBrush(Qt::red));
                    break;
                case CheckResultItem::SmallFace:
                case CheckResultItem::SmallEdge:
                    item->setForeground(0, QBrush(QColor(255, 165, 0)));
                    break;
                default:
                    item->setForeground(0, QBrush(Qt::darkBlue));
                    break;
            }
            
            if (result.type == CheckResultItem::InvalidFace || 
                result.type == CheckResultItem::SelfIntersectingFace) {
                item->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
            }
            
            if (categoryItem == faceCategory) faceCount++;
            else if (categoryItem == edgeCategory) edgeCount++;
            else if (categoryItem == gapCategory) gapCount++;
        }
    }
    
    // 删除空的类别节点
    if (faceCount == 0) {
        delete faceCategory;
        faceCategory = nullptr;
    }
    if (edgeCount == 0) {
        delete edgeCategory;
        edgeCategory = nullptr;
    }
    if (gapCount == 0) {
        delete gapCategory;
        gapCategory = nullptr;
    }
    
    m_resultsTree->expandAll();
    
    updateRepairButtonState();
}

void ParasolidCheckRepairWidget::updateCheckButtonState()
{
    m_btnCheck->setEnabled(m_parasolidData.isValid() && m_checker != nullptr);
}

void ParasolidCheckRepairWidget::updateRepairButtonState()
{
    bool hasProblems = (m_resultsTree->topLevelItemCount() > 0);
    m_btnRepair->setEnabled(hasProblems);
}

void ParasolidCheckRepairWidget::highlightResultItem(QTreeWidgetItem* item)
{
    if (item && item->text(2).toInt() > 0) {
        emit highlightObject(item->text(2).toInt());
    }
}