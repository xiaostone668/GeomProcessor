#pragma once
#ifndef GEOMETRY_CHECK_DIALOG_H
#define GEOMETRY_CHECK_DIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QCommandLinkButton>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <TopoDS_Shape.hxx>
#include "GeomChecker.h"

/**
 * @brief 几何检查对话框
 *
 * 独立的检查界面，用户可以选择要检查的类型
 */
class GeometryCheckDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GeometryCheckDialog(QWidget* parent = nullptr);
    ~GeometryCheckDialog() override;

    void setChecker(GeomChecker* checker) { m_checker = checker; }
    void setShape(const TopoDS_Shape& shape);

    // 执行检查
    bool runChecks();

signals:
    void checkingFinished(int problemCount);

private slots:
    void onCheckAll();
    void onUncheckAll();
    void onRun();
    void onExportReport();

private:
    void setupUI();
    void updateCheckOptions();
    void updateResultsTree();

private:
    GeomChecker*     m_checker = nullptr;
    TopoDS_Shape     m_currentShape;

    // Check options
    QGroupBox*       m_checkOptionsGroup = nullptr;
    QCheckBox*       m_cbDuplicateFaces = nullptr;
    QCheckBox*       m_cbInvalidFaces = nullptr;
    QCheckBox*       m_cbSmallFaces = nullptr;
    QCheckBox*       m_cbSmallEdges = nullptr;

    // Parameters
    QDoubleSpinBox*  m_toleranceSpin = nullptr;

    // Results
    QTreeWidget*     m_resultsTree = nullptr;
    QLabel*          m_problemCountLabel = nullptr;
    int              m_problemCount = 0;
};

#endif // GEOMETRY_CHECK_DIALOG_H