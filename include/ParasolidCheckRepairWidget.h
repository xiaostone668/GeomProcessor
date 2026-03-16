#pragma once
#ifndef PARASOLID_CHECK_REPAIR_WIDGET_H
#define PARASOLID_CHECK_REPAIR_WIDGET_H

#include <QWidget>
#include <QVariant>
#include <QTreeWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QListWidget>
#include <QLineEdit>
#include "GeomChecker.h"

class ParasolidChecker;

/**
 * @brief Parasolid几何检查和修复Widget
 * 
 * 采用垂直布局的现代简洁界面，作为停靠面板使用
 * UI设计与CadCheckRepairWidget一致
 */
class ParasolidCheckRepairWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParasolidCheckRepairWidget(QWidget* parent = nullptr);
    ~ParasolidCheckRepairWidget() override;

    // 设置检查器
    void setChecker(ParasolidChecker* checker);
    
    // 设置要检查的Parasolid数据
    void setShapeData(const QVariant& parasolidData);
    
    // 执行检查
    bool runChecks();

signals:
    void checkingFinished(int problemCount);
    void repairRequested(const QList<int>& problemIndices);
    void highlightObject(int index);

private slots:
    void onExpandAll();
    void onCollapseAll();
    void onSelectFromCanvas();
    void onSelectFromTree();
    void onClearSelection();
    void onCheckAll(int state);
    void onUncheckAll();
    void onRunCheck();
    void onRunRepair();
    void onResultItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onResultItemContextMenu(const QPoint& pos);
    void onExcludeItem();

private:
    void setupUI();
    void setupCheckObjectArea();
    void setupPrecisionArea();
    void setupCheckItemArea();
    void setupResultArea();
    void updateCheckOptions();
    void updateResultsTree();
    void updateCheckButtonState();
    void updateRepairButtonState();
    void highlightResultItem(QTreeWidgetItem* item);

private:
    // 检查器
    ParasolidChecker* m_checker = nullptr;
    QVariant m_parasolidData;

    // 标题区域
    QPushButton* m_btnExpandAll = nullptr;
    QPushButton* m_btnCollapseAll = nullptr;

    // 检查对象区域
    QLineEdit* m_comboSelection = nullptr;
    QListWidget* m_listSelection = nullptr;
    QPushButton* m_btnSelectFromCanvas = nullptr;
    QPushButton* m_btnSelectFromTree = nullptr;
    QPushButton* m_btnClearSelection = nullptr;

    // 精度设置区域
    QDoubleSpinBox* m_spinDistancePrecision = nullptr;
    QDoubleSpinBox* m_spinAnglePrecision = nullptr;

    // 检查项区域
    QCheckBox* m_cbCheckAll = nullptr;
    QCheckBox* m_cbTinyFace = nullptr;
    QCheckBox* m_cbSliverFace = nullptr;
    QCheckBox* m_cbSpikeFace = nullptr;
    QCheckBox* m_cbDuplicateFace = nullptr;
    QCheckBox* m_cbIntersectingFace = nullptr;
    QCheckBox* m_cbExtraPoint = nullptr;
    QCheckBox* m_cbExtraEdge = nullptr;
    QCheckBox* m_cbSmallEdge = nullptr;
    QCheckBox* m_cbGap = nullptr;

    // 结果区域
    QTreeWidget* m_resultsTree = nullptr;
    QPushButton* m_btnCheck = nullptr;
    QPushButton* m_btnRepair = nullptr;

    // 布局成员
    QGroupBox* m_checkObjectGroup = nullptr;
    QGroupBox* m_precisionGroup = nullptr;
    QGroupBox* m_checkItemGroup = nullptr;
    QGroupBox* m_resultGroup = nullptr;

    // 修复确认
    bool m_repairConfirmed = false;
};

#endif // PARASOLID_CHECK_REPAIR_WIDGET_H