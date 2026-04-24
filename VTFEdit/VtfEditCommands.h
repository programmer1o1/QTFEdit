#pragma once

#include <QString>
#include <QUndoCommand>

#include <VTFLib.h>

class MainWindow;

class SetVtfFlagsCommand final : public QUndoCommand {
public:
    SetVtfFlagsCommand(MainWindow *mw, vlUInt oldFlags, vlUInt newFlags, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow *mw_;
    vlUInt oldFlags_;
    vlUInt newFlags_;
    bool firstRun_ = true;
};

class SetVtfMinorVersionCommand final : public QUndoCommand {
public:
    SetVtfMinorVersionCommand(MainWindow *mw, int oldMinor, int newMinor, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow *mw_;
    int oldMinor_;
    int newMinor_;
    bool firstRun_ = true;
};

class SetVtfStartFrameCommand final : public QUndoCommand {
public:
    SetVtfStartFrameCommand(MainWindow *mw, unsigned int oldVal, unsigned int newVal, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow *mw_;
    unsigned int oldVal_;
    unsigned int newVal_;
    bool firstRun_ = true;
};

class SetVtfBumpScaleCommand final : public QUndoCommand {
public:
    SetVtfBumpScaleCommand(MainWindow *mw, float oldVal, float newVal, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow *mw_;
    float oldVal_;
    float newVal_;
    bool firstRun_ = true;
};

class SetVtfReflectivityCommand final : public QUndoCommand {
public:
    SetVtfReflectivityCommand(MainWindow *mw, float ox, float oy, float oz,
                              float nx, float ny, float nz, QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    MainWindow *mw_;
    float old_[3];
    float new_[3];
    bool firstRun_ = true;
};
