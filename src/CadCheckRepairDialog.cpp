#include "CadCheckRepairDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>

CadCheckRepairDialog::CadCheckRepairDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("CAD模型检查修复工具"));
    resize(450, 800);
    setupUI();
}

CadCheckRepairDialog::~CadCheckRepairDialog() = default;

void CadCheckRepairDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 标题区域
    setupTitleArea();
    mainLayout->addLayout(m_titleArea);

    // 分隔线
    auto* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line1);

    // 检查对象区域
    setupCheckObjectArea();
    mainLayout->addWidget(m_checkObjectGroup);

    // 精度设置区域
    setupPrecisionArea();
    mainLayout->addWidget(m_precisionGroup);

    // 检查项区域
    setupCheckItemArea();
    mainLayout->addWidget(m_checkItemGroup);

    // 结果区域
    setupResultArea();
    mainLayout->addWidget(m_resultGroup);
    
    mainLayout->addStretch();
}

void CadCheckRepairDialog::setupTitleArea()
{
    m_titleArea = new QHBoxLayout();
    
    // 左侧标题
    auto* titleLabel = new QLabel(
        QString::fromUtf8("间隙：体内的缝隙（共点/非共点等）"), this);
    titleLabel->setStyleSheet("font-weight:bold; font-size:12px;");
    m_titleArea->addWidget(titleLabel);
    
    m_titleArea->addStretch();
    
    // 右侧按钮
    m_btnExpandAll = new QPushButton(QString::fromUtf8("全部展开"), this);
    m_btnCollapseAll = new QPushButton(QString::fromUtf8("全部折叠"), this);
    m_btnExpandAll->setMaximumWidth(80);
    m_btnCollapseAll->setMaximumWidth(80);
    
    connect(m_btnExpandAll, &QPushButton::clicked, this, &CadCheckRepairDialog::onExpandAll);
    connect(m_btnCollapseAll, &QPushButton::clicked, this, &CadCheckRepairDialog::onCollapseAll);
    
    m_titleArea->addWidget(m_btnExpandAll);
    m_titleArea->addWidget(m_btnCollapseAll);
}

void CadCheckRepairDialog::setupCheckObjectArea()
{
    m_checkObjectGroup = new QGroupBox(QString::fromUtf8("检查对象"), this);
    auto* groupLayout = new QVBoxLayout(m_checkObjectGroup);
    
    // 第一行：输入选择
    auto* inputLayout = new QHBoxLayout();
    auto* labelSelected = new QLabel(QString::fromUtf8("已选择"), this);
    m_comboSelection = new QComboBox(this);
    m_comboSelection->setEditable(true);
    // Qt 5.6.3 没有setPlaceholderText方法
    m_comboSelection->setEditText(QString::fromUtf8("多选输入框，必选项"));
    
    m_btnSelectFromCanvas = new QPushButton(QString::fromUtf8("从画布选择"), this);
    m_btnSelectFromTree = new QPushButton(QString::fromUtf8("从结构树选择"), this);
    
    connect(m_btnSelectFromCanvas, &QPushButton::clicked, this, &CadCheckRepairDialog::onSelectFromCanvas);
    connect(m_btnSelectFromTree, &QPushButton::clicked, this, &CadCheckRepairDialog::onSelectFromTree);
    
    inputLayout->addWidget(labelSelected);
    inputLayout->addWidget(m_comboSelection, 1);
    inputLayout->addWidget(m_btnSelectFromCanvas);
    inputLayout->addWidget(m_btnSelectFromTree);
    groupLayout->addLayout(inputLayout);
    
    // 第二行：已选列表
    m_listSelection = new QListWidget(this);
    m_listSelection->setMaximumHeight(100);
    m_listSelection->setSelectionMode(QAbstractItemView::ExtendedSelection);
    // 添加示例项
    m_listSelection->addItem(QString::fromUtf8("实体1"));
    m_listSelection->addItem(QString::fromUtf8("曲面体2"));
    groupLayout->addWidget(m_listSelection);
    
    // 第三行：清空选择按钮
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_btnClearSelection = new QPushButton(QString::fromUtf8("清空选择"), this);
    connect(m_btnClearSelection, &QPushButton::clicked, this, &CadCheckRepairDialog::onClearSelection);
    buttonLayout->addWidget(m_btnClearSelection);
    groupLayout->addLayout(buttonLayout);
}

void CadCheckRepairDialog::setupPrecisionArea()
{
    m_precisionGroup = new QGroupBox(QString::fromUtf8("精度设置"), this);
    auto* groupLayout = new QGridLayout(m_precisionGroup);
    
    // 距离精度
    groupLayout->addWidget(new QLabel(QString::fromUtf8("距离精度："), this), 0, 0);
    m_spinDistancePrecision = new QDoubleSpinBox(this);
    m_spinDistancePrecision->setRange(0.001, 100.0);
    m_spinDistancePrecision->setValue(0.1);
    m_spinDistancePrecision->setSingleStep(0.1);
    m_spinDistancePrecision->setDecimals(3);
    groupLayout->addWidget(m_spinDistancePrecision, 0, 1);
    groupLayout->addWidget(new QLabel(QString::fromUtf8("mm"), this), 0, 2);
    
    // 角度精度
    groupLayout->addWidget(new QLabel(QString::fromUtf8("角度精度："), this), 1, 0);
    m_spinAnglePrecision = new QDoubleSpinBox(this);
    m_spinAnglePrecision->setRange(0.01, 180.0);
    m_spinAnglePrecision->setValue(0.01);
    m_spinAnglePrecision->setSingleStep(0.1);
    m_spinAnglePrecision->setDecimals(2);
    groupLayout->addWidget(m_spinAnglePrecision, 1, 1);
    groupLayout->addWidget(new QLabel(QString::fromUtf8("度"), this), 1, 2);
}

void CadCheckRepairDialog::setupCheckItemArea()
{
    m_checkItemGroup = new QGroupBox(QString::fromUtf8("检查项"), this);
    auto* groupLayout = new QVBoxLayout(m_checkItemGroup);
    
    // 全选复选框
    m_cbCheckAll = new QCheckBox(QString::fromUtf8("全选"), this);
    m_cbCheckAll->setChecked(true);
    connect(m_cbCheckAll, &QCheckBox::stateChanged, this, &CadCheckRepairDialog::onCheckAll);
    groupLayout->addWidget(m_cbCheckAll);
    
    // 面类型子分组
    auto* faceGroup = new QGroupBox(QString::fromUtf8("面类型"), this);
    auto* faceLayout = new QGridLayout(faceGroup);
    
    m_cbTinyFace = new QCheckBox(QString::fromUtf8("微小面"), this);
    m_cbSliverFace = new QCheckBox(QString::fromUtf8("狭长面"), this);
    m_cbSpikeFace = new QCheckBox(QString::fromUtf8("尖刺面"), this);
    m_cbDuplicateFace = new QCheckBox(QString::fromUtf8("重复面"), this);
    m_cbIntersectingFace = new QCheckBox(QString::fromUtf8("相交面"), this);
    
    // 默认选中所有面类型
    m_cbTinyFace->setChecked(true);
    m_cbSliverFace->setChecked(true);
    m_cbSpikeFace->setChecked(true);
    m_cbDuplicateFace->setChecked(true);
    m_cbIntersectingFace->setChecked(true);
    
    // 设置属性用于识别
    m_cbTinyFace->setProperty("checkOption", (int)GeomChecker::CheckSmallFaces);
    m_cbSliverFace->setProperty("checkOption", (int)GeomChecker::CheckSliverFaces);
    m_cbSpikeFace->setProperty("checkOption", (int)GeomChecker::CheckSpikes);
    m_cbDuplicateFace->setProperty("checkOption", (int)GeomChecker::CheckDuplicateFaces);
    m_cbIntersectingFace->setProperty("checkOption", (int)GeomChecker::CheckIntersectingFaces);
    
    faceLayout->addWidget(m_cbTinyFace, 0, 0);
    faceLayout->addWidget(m_cbSliverFace, 0, 1);
    faceLayout->addWidget(m_cbSpikeFace, 1, 0);
    faceLayout->addWidget(m_cbDuplicateFace, 1, 1);
    faceLayout->addWidget(m_cbIntersectingFace, 2, 0, 1, 2);
    
    groupLayout->addWidget(faceGroup);
    
    // 边类型子分组
    auto* edgeGroup = new QGroupBox(QString::fromUtf8("边类型"), this);
    auto* edgeLayout = new QGridLayout(edgeGroup);
    
    m_cbExtraPoint = new QCheckBox(QString::fromUtf8("余点"), this);
    m_cbExtraEdge = new QCheckBox(QString::fromUtf8("余边"), this);
    m_cbSmallEdge = new QCheckBox(QString::fromUtf8("微小边"), this);
    m_cbGap = new QCheckBox(QString::fromUtf8("间隙"), this);
    
    // 默认选中所有边类型
    m_cbExtraPoint->setChecked(true);
    m_cbExtraEdge->setChecked(true);
    m_cbSmallEdge->setChecked(true);
    m_cbGap->setChecked(true);
    
    // 设置属性用于识别
    m_cbExtraPoint->setProperty("checkOption", (int)GeomChecker::CheckRedundantVertices);
    m_cbExtraEdge->setProperty("checkOption", (int)GeomChecker::CheckRedundantEdges);
    m_cbSmallEdge->setProperty("checkOption", (int)GeomChecker::CheckSmallEdges);
    m_cbGap->setProperty("checkOption", (int)GeomChecker::CheckGaps);
    
    edgeLayout->addWidget(m_cbExtraPoint, 0, 0);
    edgeLayout->addWidget(m_cbExtraEdge, 0, 1);
    edgeLayout->addWidget(m_cbSmallEdge, 1, 0);
    edgeLayout->addWidget(m_cbGap, 1, 1);
    
    groupLayout->addWidget(edgeGroup);
    
    // 检查按钮
    m_btnCheck = new QPushButton(QString::fromUtf8("检查"), this);
    m_btnCheck->setStyleSheet(QString::fromUtf8(
        "QPushButton {"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #45a049;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    ));
    connect(m_btnCheck, &QPushButton::clicked, this, &CadCheckRepairDialog::onRunCheck);
    groupLayout->addWidget(m_btnCheck);
}

void CadCheckRepairDialog::setupResultArea()
{
    m_resultGroup = new QGroupBox(QString::fromUtf8("结果"), this);
    auto* groupLayout = new QVBoxLayout(m_resultGroup);
    
    // 结果树
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels(QStringList() 
        << QString::fromUtf8("问题类型") 
        << QString::fromUtf8("描述") 
        << QString::fromUtf8("索引"));
    m_resultsTree->setColumnWidth(0, 100);
    m_resultsTree->setColumnWidth(1, 180);
    m_resultsTree->setColumnWidth(2, 60);
    m_resultsTree->setAlternatingRowColors(true);
    m_resultsTree->setRootIsDecorated(true);
    m_resultsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked,
            this, &CadCheckRepairDialog::onResultItemDoubleClicked);
    m_resultsTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_resultsTree, &QTreeWidget::customContextMenuRequested,
            this, &CadCheckRepairDialog::onResultItemContextMenu);
    
    groupLayout->addWidget(m_resultsTree);
    
    // 修复按钮
    m_btnRepair = new QPushButton(QString::fromUtf8("修复"), this);
    m_btnRepair->setStyleSheet(QString::fromUtf8(
        "QPushButton {"
        "  background-color: #2196F3;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0b7dda;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    ));
    connect(m_btnRepair, &QPushButton::clicked, this, &CadCheckRepairDialog::onRunRepair);
    groupLayout->addWidget(m_btnRepair);
    
    // 初始状态：修复按钮禁用
    m_btnRepair->setEnabled(false);
}

void CadCheckRepairDialog::setShape(const TopoDS_Shape& shape)
{
    m_currentShape = shape;
    m_resultsTree->clear();
    m_btnRepair->setEnabled(false);
}

bool CadCheckRepairDialog::runChecks()
{
    if (!m_checker || m_currentShape.IsNull()) {
        return false;
    }

    updateCheckOptions();
    m_resultsTree->clear();
    
    return m_checker->checkShape(m_currentShape);
}

void CadCheckRepairDialog::onExpandAll()
{
    m_resultsTree->expandAll();
}

void CadCheckRepairDialog::onCollapseAll()
{
    m_resultsTree->collapseAll();
}

void CadCheckRepairDialog::onSelectFromCanvas()
{
    QMessageBox::information(this, QString::fromUtf8("提示"), 
        QString::fromUtf8("从3D画布选择对象的功能待实现"));
}

void CadCheckRepairDialog::onSelectFromTree()
{
    QMessageBox::information(this, QString::fromUtf8("提示"), 
        QString::fromUtf8("从结构树选择对象的功能待实现"));
}

void CadCheckRepairDialog::onClearSelection()
{
    m_listSelection->clear();
    m_comboSelection->clear();
}

void CadCheckRepairDialog::onCheckAll(int state)
{
    bool checked = (state == Qt::Checked);
    
    // 面类型
    m_cbTinyFace->setChecked(checked);
    m_cbSliverFace->setChecked(checked);
    m_cbSpikeFace->setChecked(checked);
    m_cbDuplicateFace->setChecked(checked);
    m_cbIntersectingFace->setChecked(checked);
    
    // 边类型
    m_cbExtraPoint->setChecked(checked);
    m_cbExtraEdge->setChecked(checked);
    m_cbSmallEdge->setChecked(checked);
    m_cbGap->setChecked(checked);
}

void CadCheckRepairDialog::onUncheckAll()
{
    m_cbCheckAll->setChecked(false);
}

void CadCheckRepairDialog::onRunCheck()
{
    if (!m_checker || m_currentShape.IsNull()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("请先加载几何体"));
        return;
    }

    m_resultsTree->clear();

    if (m_checker->checkShape(m_currentShape)) {
        updateResultsTree();
        
        int numProblems = m_checker->getProblemCount();
        QMessageBox::information(this, QString::fromUtf8("完成"), 
            QString::fromUtf8("检查完成！共发现 %1 个问题").arg(numProblems));
        emit checkingFinished(numProblems);
    }
}

void CadCheckRepairDialog::onRunRepair()
{
    // 显示确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QString::fromUtf8("确认修复"),
        QString::fromUtf8("将一键自动修复错误问题，是否继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 收集未被排除的问题索引
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
            QString::fromUtf8("修复功能待实现"));
    }
}

void CadCheckRepairDialog::onResultItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (!item) return;
    
    highlightResultItem(item);
}

void CadCheckRepairDialog::onResultItemContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_resultsTree->itemAt(pos);
    if (!item || !item->parent()) {
        // 不是具体问题项，不显示菜单
        return;
    }
    
    QMenu contextMenu(this);
    QAction* excludeAction = contextMenu.addAction(QString::fromUtf8("排除此项"));
    
    QAction* action = contextMenu.exec(m_resultsTree->viewport()->mapToGlobal(pos));
    if (action == excludeAction) {
        onExcludeItem();
    }
}

void CadCheckRepairDialog::onExcludeItem()
{
    QTreeWidgetItem* item = m_resultsTree->currentItem();
    if (!item || !item->parent()) {
        return;
    }
    
    // 标记为已排除（灰色背景）
    item->setDisabled(true);
    item->setForeground(0, QBrush(Qt::gray));
    
    updateRepairButtonState();
}

void CadCheckRepairDialog::updateCheckOptions()
{
    if (!m_checker) return;

    GeomChecker::CheckOptions options = GeomChecker::CheckNone;

    // 面类型
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
    
    // 边类型
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

void CadCheckRepairDialog::updateResultsTree()
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
    
    // 统计各类别的项目数量
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
            item->setText(1, result.getDisplayString());
            item->setText(2, QString::number(result.index));
            
            // 设置颜色
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
            
            // 设置图标（根据类型）
            if (result.type == CheckResultItem::InvalidFace || 
                result.type == CheckResultItem::SelfIntersectingFace) {
                item->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
            }
            
            if (categoryItem == faceCategory) faceCount++;
            else if (categoryItem == edgeCategory) edgeCount++;
            else if (categoryItem == gapCategory) gapCount++;
        }
    }
    
    // 移除空类别
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
    
    // 展开所有类别
    m_resultsTree->expandAll();
    
    updateRepairButtonState();
}

void CadCheckRepairDialog::updateCheckButtonState()
{
    bool hasSelection = (m_listSelection->count() > 0);
    m_btnCheck->setEnabled(hasSelection);
}

void CadCheckRepairDialog::updateRepairButtonState()
{
    bool hasProblems = (m_resultsTree->topLevelItemCount() > 0);
    m_btnRepair->setEnabled(hasProblems);
}

void CadCheckRepairDialog::highlightResultItem(QTreeWidgetItem* item)
{
    // 触发高亮信号
    if (item && item->text(2).toInt() > 0) {
        emit highlightObject(item->text(2).toInt());
    }
}