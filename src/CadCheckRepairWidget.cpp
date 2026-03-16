#include "CadCheckRepairWidget.h"
#include "GeomChecker.h"
#include "ParasolidChecker.h"

#include <QVariant>
#include <QVBoxLayout>

// OCC
#include <TopExp_Explorer.hxx>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>

CadCheckRepairWidget::CadCheckRepairWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

CadCheckRepairWidget::~CadCheckRepairWidget() = default;

void CadCheckRepairWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    setupCheckObjectArea();
    mainLayout->addWidget(m_checkObjectGroup);
    setupCheckItemArea();
    mainLayout->addWidget(m_checkItemGroup);
    setupResultArea();
    mainLayout->addWidget(m_resultGroup);
    
    mainLayout->addStretch();
    m_btnCheck->setEnabled(true);
}

void CadCheckRepairWidget::setupCheckObjectArea()
{
    m_checkObjectGroup = new QGroupBox(QString::fromUtf8("检查对象"), this);
    auto* groupLayout = new QVBoxLayout(m_checkObjectGroup);
    
    auto* inputLayout = new QHBoxLayout();
    auto* labelSelected = new QLabel(QString::fromUtf8("已选择"), this);
    m_comboSelection = new QComboBox(this);
    m_comboSelection->setEditable(true);
    m_comboSelection->setEditText(QString::fromUtf8("全部几何体"));
    
    inputLayout->addWidget(labelSelected);
    inputLayout->addWidget(m_comboSelection, 1);
    groupLayout->addLayout(inputLayout);
    
    auto* distLayout = new QHBoxLayout();
    auto* labelDist = new QLabel(QString::fromUtf8("距离精度："), this);
    m_spinDistancePrecision = new QDoubleSpinBox(this);
    m_spinDistancePrecision->setRange(0.001, 100.0);
    m_spinDistancePrecision->setValue(0.1);
    m_spinDistancePrecision->setSingleStep(0.1);
    m_spinDistancePrecision->setDecimals(3);
    auto* labelDistUnit = new QLabel(QString::fromUtf8("mm"), this);
    
    distLayout->addStretch();
    distLayout->addWidget(labelDist);
    distLayout->addWidget(m_spinDistancePrecision);
    distLayout->addWidget(labelDistUnit);
    distLayout->addStretch();
    groupLayout->addLayout(distLayout);
    
    auto* angleLayout = new QHBoxLayout();
    auto* labelAngle = new QLabel(QString::fromUtf8("角度精度："), this);
    m_spinAnglePrecision = new QDoubleSpinBox(this);
    m_spinAnglePrecision->setRange(0.01, 180.0);
    m_spinAnglePrecision->setValue(0.01);
    m_spinAnglePrecision->setSingleStep(0.1);
    m_spinAnglePrecision->setDecimals(2);
    auto* labelAngleUnit = new QLabel(QString::fromUtf8("度"), this);
    
    angleLayout->addStretch();
    angleLayout->addWidget(labelAngle);
    angleLayout->addWidget(m_spinAnglePrecision);
    angleLayout->addWidget(labelAngleUnit);
    angleLayout->addStretch();
    groupLayout->addLayout(angleLayout);
}

void CadCheckRepairWidget::setupCheckItemArea()
{
    m_checkItemGroup = new QGroupBox(QString::fromUtf8("检查项"), this);
    auto* groupLayout = new QVBoxLayout(m_checkItemGroup);
    
    m_cbCheckAll = new QCheckBox(QString::fromUtf8("全选"), this);
    m_cbCheckAll->setChecked(true);
    connect(m_cbCheckAll, &QCheckBox::stateChanged, this, &CadCheckRepairWidget::onCheckAll);
    groupLayout->addWidget(m_cbCheckAll);
    
    auto* faceGroup = new QGroupBox(QString::fromUtf8("面类型"), this);
    auto* faceLayout = new QGridLayout(faceGroup);
    
    m_cbTinyFace = new QCheckBox(QString::fromUtf8("微小面"), this);
    m_cbSliverFace = new QCheckBox(QString::fromUtf8("狭长面"), this);
    m_cbSpikeFace = new QCheckBox(QString::fromUtf8("尖刺"), this);
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
    
    faceLayout->addWidget(m_cbTinyFace, 0, 0);
    faceLayout->addWidget(m_cbSliverFace, 0, 1);
    faceLayout->addWidget(m_cbSpikeFace, 0, 2);
    faceLayout->addWidget(m_cbDuplicateFace, 1, 0, 1, 2);
    faceLayout->addWidget(m_cbIntersectingFace, 1, 2);
    
    groupLayout->addWidget(faceGroup);
    
    auto* edgeGroup = new QGroupBox(QString::fromUtf8("边类型"), this);
    auto* edgeLayout = new QGridLayout(edgeGroup);
    
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
    
    edgeLayout->addWidget(m_cbExtraPoint, 0, 0);
    edgeLayout->addWidget(m_cbExtraEdge, 0, 1);
    edgeLayout->addWidget(m_cbSmallEdge, 0, 2);
    edgeLayout->addWidget(m_cbGap, 1, 0, 1, 3);
    
    groupLayout->addWidget(edgeGroup);
}

void CadCheckRepairWidget::setupResultArea()
{
    m_resultGroup = new QGroupBox(QString::fromUtf8("结果"), this);
    auto* groupLayout = new QVBoxLayout(m_resultGroup);
    
    auto* topButtonLayout = new QHBoxLayout();
    topButtonLayout->addStretch();
    m_btnRepair = new QPushButton(QString::fromUtf8("修复"), this);
    m_btnRepair->setMaximumWidth(80);
    m_btnRepair->setStyleSheet(QString::fromUtf8(
        "QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 6px 12px; }"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    ));
    connect(m_btnRepair, &QPushButton::clicked, this, &CadCheckRepairWidget::onRunRepair);
    topButtonLayout->addWidget(m_btnRepair);
    groupLayout->addLayout(topButtonLayout);
    
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels(QStringList() << QString::fromUtf8("问题类型") << QString::fromUtf8("描述") << QString::fromUtf8("索引"));
    m_resultsTree->setColumnWidth(0, 80);
    m_resultsTree->setColumnWidth(1, 180);
    m_resultsTree->setColumnWidth(2, 50);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setRootIsDecorated(true);
    m_resultsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked, this, &CadCheckRepairWidget::onResultItemDoubleClicked);
    m_resultsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_resultsTree, &QTreeWidget::customContextMenuRequested, this, &CadCheckRepairWidget::onResultItemContextMenu);
    
    groupLayout->addWidget(m_resultsTree);
    
    auto* bottomButtonLayout = new QHBoxLayout();
    bottomButtonLayout->addStretch();
    m_btnCheck = new QPushButton(QString::fromUtf8("检查"), this);
    m_btnCheck->setMaximumWidth(80);
    m_btnCheck->setStyleSheet(QString::fromUtf8(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 6px 12px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    ));
    connect(m_btnCheck, &QPushButton::clicked, this, &CadCheckRepairWidget::onRunCheck);
    bottomButtonLayout->addWidget(m_btnCheck);
    groupLayout->addLayout(bottomButtonLayout);
    
    m_btnRepair->setEnabled(false);
}

void CadCheckRepairWidget::setShape(const TopoDS_Shape& shape)
{
    m_currentShape = shape;
    m_resultsTree->clear();
    m_btnRepair->setEnabled(false);
}

void CadCheckRepairWidget::setChecker(void* checker)
{
    m_checker = checker;
    m_isParasolidChecker = (reinterpret_cast<ParasolidChecker*>(checker) != nullptr);
}

void CadCheckRepairWidget::setShapeData(const QVariant& parasolidData)
{
    m_parasolidData = parasolidData;
    m_resultsTree->clear();
    m_btnRepair->setEnabled(false);
}

bool CadCheckRepairWidget::runChecks()
{
    if (!m_checker) return false;
    
    updateCheckOptions();
    m_resultsTree->clear();
    
    if (m_isParasolidChecker) {
        return reinterpret_cast<ParasolidChecker*>(m_checker)->checkShape(m_parasolidData);
    } else {
        return reinterpret_cast<GeomChecker*>(m_checker)->checkShape(m_currentShape);
    }
}

void CadCheckRepairWidget::onExpandAll()
{
    m_resultsTree->expandAll();
}

void CadCheckRepairWidget::onCollapseAll()
{
    m_resultsTree->collapseAll();
}

void CadCheckRepairWidget::onSelectFromCanvas()
{
    QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("从3D画布选择对象的功能待实现"));
}

void CadCheckRepairWidget::onSelectFromTree()
{
    QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("从结构树选择对象的功能待实现"));
}

void CadCheckRepairWidget::onClearSelection()
{
    m_comboSelection->clear();
}

void CadCheckRepairWidget::onCheckAll(int state)
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

void CadCheckRepairWidget::onUncheckAll()
{
    m_cbCheckAll->setChecked(false);
}

void CadCheckRepairWidget::onRunCheck()
{
    if (!m_checker) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("检查器未初始化"));
        return;
    }

    if (m_isParasolidChecker) {
        if (m_parasolidData.isNull()) {
            QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("Parasolid模式：数据未加载（框架版本）"));
            return;
        }
    } else {
        if (m_currentShape.IsNull()) {
            int numFaces = 0, numEdges = 0;
            TopExp_Explorer faceExp(m_currentShape, TopAbs_FACE);
            for (; faceExp.More(); faceExp.Next()) numFaces++;
            TopExp_Explorer edgeExp(m_currentShape, TopAbs_EDGE);
            for (; edgeExp.More(); edgeExp.Next()) numEdges++;
            
            QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载STEP文件\n当前状态：%1个面，%2条边").arg(numFaces).arg(numEdges));
            return;
        }
    }

    m_resultsTree->clear();

    bool success = false;
    if (m_isParasolidChecker) {
        success = reinterpret_cast<ParasolidChecker*>(m_checker)->checkShape(m_parasolidData);
    } else {
        success = reinterpret_cast<GeomChecker*>(m_checker)->checkShape(m_currentShape);
    }
    
    if (success) {
        updateResultsTree();
        
        int numProblems = 0;
        if (m_isParasolidChecker) {
            numProblems = reinterpret_cast<ParasolidChecker*>(m_checker)->getProblemCount();
        } else {
            numProblems = reinterpret_cast<GeomChecker*>(m_checker)->getProblemCount();
        }
        emit checkingFinished(numProblems);
    }
}

void CadCheckRepairWidget::onRunRepair()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, QString::fromUtf8("确认修复"), QString::fromUtf8("将一键自动修复错误问题，是否继续？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QList<int> problemIndices;
        for (int i = 0; i < m_resultsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* categoryItem = m_resultsTree->topLevelItem(i);
            for (int j = 0; j < categoryItem->childCount(); ++j) {
                QTreeWidgetItem* item = categoryItem->child(j);
                if (!item->isDisabled()) item->text(2).toInt();
            }
        }
        
        emit repairRequested(problemIndices);
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("修复功能待实现"));
    }
}

void CadCheckRepairWidget::onResultItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (item) highlightResultItem(item);
}

void CadCheckRepairWidget::onResultItemContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_resultsTree->itemAt(pos);
    if (!item || !item->parent()) return;
    
    QMenu contextMenu(this);
    QAction* excludeAction = contextMenu.addAction(QString::fromUtf8("排除此项"));
    
    QAction* action = contextMenu.exec(m_resultsTree->viewport()->mapToGlobal(pos));
    if (action == excludeAction) onExcludeItem();
}

void CadCheckRepairWidget::onExcludeItem()
{
    QTreeWidgetItem* item = m_resultsTree->currentItem();
    if (!item || !item->parent()) return;
    
    item->setDisabled(true);
    item->setForeground(0, QBrush(Qt::gray));
    updateRepairButtonState();
}

void CadCheckRepairWidget::updateCheckOptions()
{
    if (!m_checker) return;

    GeomChecker::CheckOptions options = GeomChecker::CheckNone;

    if (m_cbTinyFace->isChecked()) options |= GeomChecker::CheckSmallFaces;
    if (m_cbSliverFace->isChecked()) options |= GeomChecker::CheckSliverFaces;
    if (m_cbSpikeFace->isChecked()) options |= GeomChecker::CheckSpikes;
    if (m_cbDuplicateFace->isChecked()) options |= GeomChecker::CheckDuplicateFaces;
    if (m_cbIntersectingFace->isChecked()) options |= GeomChecker::CheckIntersectingFaces;
    if (m_cbExtraPoint->isChecked()) options |= GeomChecker::CheckRedundantVertices;
    if (m_cbExtraEdge->isChecked()) options |= GeomChecker::CheckRedundantEdges;
    if (m_cbSmallEdge->isChecked()) options |= GeomChecker::CheckSmallEdges;
    if (m_cbGap->isChecked()) options |= GeomChecker::CheckGaps;

    if (m_isParasolidChecker) {
        reinterpret_cast<ParasolidChecker*>(m_checker)->setCheckOptions(options);
    } else {
        reinterpret_cast<GeomChecker*>(m_checker)->setCheckOptions(options);
    }
}

void CadCheckRepairWidget::updateResultsTree()
{
    m_resultsTree->clear();
    
    const QList<CheckResultItem>* results = nullptr;
    if (m_isParasolidChecker) {
        results = &reinterpret_cast<ParasolidChecker*>(m_checker)->getResults();
    } else {
        results = &reinterpret_cast<GeomChecker*>(m_checker)->getResults();
    }
    
    QTreeWidgetItem* faceCategory = new QTreeWidgetItem(m_resultsTree);
    faceCategory->setText(0, QString::fromUtf8("面类型"));
    
    QTreeWidgetItem* edgeCategory = new QTreeWidgetItem(m_resultsTree);
    edgeCategory->setText(0, QString::fromUtf8("边类型"));
    
    QTreeWidgetItem* gapCategory = new QTreeWidgetItem(m_resultsTree);
    gapCategory->setText(0, QString::fromUtf8("间隙"));
    
    int faceCount = 0, edgeCount = 0, gapCount = 0;
    
    for (const auto& result : *results) {
        QTreeWidgetItem* categoryItem = nullptr;
        
        switch (result.type) {
            case CheckResultItem::SmallFace:
            case CheckResultItem::SliverFace:
            case CheckResultItem::Spike:
            case CheckResultItem::DuplicateFace:
            case CheckResultItem::IntersectingFace:
            case CheckResultItem::InvalidFace:
            case CheckResultItem::SelfIntersectingFace:
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
    
    if (faceCount == 0) delete faceCategory;
    if (edgeCount == 0) delete edgeCategory;
    if (gapCount == 0) delete gapCategory;
    
    m_resultsTree->expandAll();
    updateRepairButtonState();
}

void CadCheckRepairWidget::updateCheckButtonState()
{
    m_btnCheck->setEnabled(true);
}

void CadCheckRepairWidget::updateRepairButtonState()
{
    m_btnRepair->setEnabled(m_resultsTree->topLevelItemCount() > 0);
}

void CadCheckRepairWidget::highlightResultItem(QTreeWidgetItem* item)
{
    if (item && item->text(2).toInt() > 0) emit highlightObject(item->text(2).toInt());
}