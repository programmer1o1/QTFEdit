#include "VtfEditCommands.h"

#include "MainWindow.h"

SetVtfFlagsCommand::SetVtfFlagsCommand(MainWindow *mw, vlUInt oldFlags, vlUInt newFlags, QUndoCommand *parent)
    : QUndoCommand(parent), mw_(mw), oldFlags_(oldFlags), newFlags_(newFlags) {
    setText(QString("Edit VTF flags (0x%1 → 0x%2)")
                .arg(oldFlags_, 8, 16, QChar('0'))
                .arg(newFlags_, 8, 16, QChar('0')));
}

void SetVtfFlagsCommand::undo() { if(mw_) mw_->applyFlagsForUndo(oldFlags_); }
void SetVtfFlagsCommand::redo() {
    if(!mw_) return;
    if(firstRun_) { firstRun_ = false; mw_->applyFlagsForUndo(newFlags_); return; }
    mw_->applyFlagsForUndo(newFlags_);
}

SetVtfMinorVersionCommand::SetVtfMinorVersionCommand(MainWindow *mw, int oldMinor, int newMinor, QUndoCommand *parent)
    : QUndoCommand(parent), mw_(mw), oldMinor_(oldMinor), newMinor_(newMinor) {
    setText(QString("Set minor version (%1 → %2)").arg(oldMinor_).arg(newMinor_));
}
void SetVtfMinorVersionCommand::undo() { if(mw_) mw_->applyMinorVersionForUndo(oldMinor_); }
void SetVtfMinorVersionCommand::redo() {
    if(!mw_) return;
    if(firstRun_) { firstRun_ = false; mw_->applyMinorVersionForUndo(newMinor_); return; }
    mw_->applyMinorVersionForUndo(newMinor_);
}

SetVtfStartFrameCommand::SetVtfStartFrameCommand(MainWindow *mw, unsigned int oldVal, unsigned int newVal, QUndoCommand *parent)
    : QUndoCommand(parent), mw_(mw), oldVal_(oldVal), newVal_(newVal) {
    setText(QString("Set start frame (%1 → %2)").arg(oldVal_).arg(newVal_));
}
void SetVtfStartFrameCommand::undo() { if(mw_) mw_->applyStartFrameForUndo(oldVal_); }
void SetVtfStartFrameCommand::redo() {
    if(!mw_) return;
    if(firstRun_) { firstRun_ = false; mw_->applyStartFrameForUndo(newVal_); return; }
    mw_->applyStartFrameForUndo(newVal_);
}

SetVtfBumpScaleCommand::SetVtfBumpScaleCommand(MainWindow *mw, float oldVal, float newVal, QUndoCommand *parent)
    : QUndoCommand(parent), mw_(mw), oldVal_(oldVal), newVal_(newVal) {
    setText(QString("Set bumpmap scale (%1 → %2)")
                .arg(static_cast<double>(oldVal_), 0, 'g', 4)
                .arg(static_cast<double>(newVal_), 0, 'g', 4));
}
void SetVtfBumpScaleCommand::undo() { if(mw_) mw_->applyBumpScaleForUndo(oldVal_); }
void SetVtfBumpScaleCommand::redo() {
    if(!mw_) return;
    if(firstRun_) { firstRun_ = false; mw_->applyBumpScaleForUndo(newVal_); return; }
    mw_->applyBumpScaleForUndo(newVal_);
}

SetVtfReflectivityCommand::SetVtfReflectivityCommand(MainWindow *mw, float ox, float oy, float oz,
                                                     float nx, float ny, float nz, QUndoCommand *parent)
    : QUndoCommand(parent), mw_(mw) {
    old_[0] = ox; old_[1] = oy; old_[2] = oz;
    new_[0] = nx; new_[1] = ny; new_[2] = nz;
    setText("Set reflectivity");
}
void SetVtfReflectivityCommand::undo() { if(mw_) mw_->applyReflectivityForUndo(old_[0], old_[1], old_[2]); }
void SetVtfReflectivityCommand::redo() {
    if(!mw_) return;
    if(firstRun_) { firstRun_ = false; mw_->applyReflectivityForUndo(new_[0], new_[1], new_[2]); return; }
    mw_->applyReflectivityForUndo(new_[0], new_[1], new_[2]);
}
