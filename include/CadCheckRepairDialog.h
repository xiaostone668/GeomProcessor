#pragma once
#ifndef CAD_CHECK_REPAIR_DIALOG_H
#define CAD_CHECK_REPAIR_DIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <TopoDS_Shape.hxx>
#include "GeomChecker.h"

/**
 * @brief CAD模型检查修复工具界面
 *
 * 采用垂直布局的现代简洁界面风格
 */
class CadCheckRepairDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CadCheckRepairDialog(QWidget* parent = nullptr);
    ~CadCheckRepairDialog() override;

    void setChecker(GeomChecker* checker) { m_checker = checker; }
    void setShape(const TopoDS_Shape& shape);

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
    void setupTitleArea();
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
    GeomChecker*     m_checker = nullptr;
    TopoDS_Shape     m_currentShape;

    // 标题区域
    QPushButton*     m_btnExpandAll = nullptr;
    QPushButton*     m_btnCollapseAll = nullptr;

    // 检查对象区域
    QComboBox*       m_comboSelection = nullptr;
    QListWidget*     m_listSelection = nullptr;
    QPushButton*     m_btnSelectFromCanvas = nullptr;
    QPushButton*     m_btnSelectFromTree = nullptr;
    QPushButton*     m_btnClearSelection = nullptr;

    // 精度设置区域
    QDoubleSpinBox*  m_spinDistancePrecision = nullptr;
    QDoubleSpinBox*  m_spinAnglePrecision = nullptr;

    // 检查项区域
    QCheckBox*       m_cbCheckAll = nullptr;
    QCheckBox*       m_cbTinyFace = nullptr;
    QCheckBox*       m_cbSliverFace = nullptr;
    QCheckBox*       m_cbSpikeFace = nullptr;
    QCheckBox*       m_cbDuplicateFace = nullptr;
    QCheckBox*       m_cbIntersectingFace = nullptr;
    QCheckBox*       m_cbExtraPoint = nullptr;
    QCheckBox*       m_cbExtraEdge = nullptr;
    QCheckBox*       m_cbSmallEdge = nullptr;
    QCheckBox*       m_cbGap = nullptr;

    // 结果区域
    QTreeWidget*     m_resultsTree = nullptr;
    QPushButton*     m_btnCheck = nullptr;
    QPushButton*     m_btnRepair = nullptr;

    // 布局成员
    QHBoxLayout*     m_titleArea = nullptr;
    QGroupBox*       m_checkObjectGroup = nullptr;
    QGroupBox*       m_precisionGroup = nullptr;
    QGroupBox*       m_checkItemGroup = nullptr;
    QGroupBox*       m_resultGroup = nullptr;

    // 修复确认
    bool             m_repairConfirmed = false;
};

#endif // CAD_CHECK_REPAIR_DIALOG_H