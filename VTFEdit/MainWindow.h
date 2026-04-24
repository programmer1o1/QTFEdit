#pragma once

#include <cstdint>
#include <limits>

#include <QColor>
#include <QDateTime>
#include <QImage>
#include <QMainWindow>
#include <QStringList>
#include <QTextDocument>

#include <VTFLib.h>

class QAction;
class QComboBox;
class QLabel;
class QMenu;
class QSpinBox;
class QScrollArea;
class QTimer;
class QPlainTextEdit;
class VmtHighlighter;
class QTreeWidget;
class QDockWidget;
class QToolButton;
class QSlider;
class QButtonGroup;
class QToolBar;
class QFileSystemWatcher;
class QUndoStack;
class Toast;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool openVtf(const QString &path);
    bool openPath(const QString &path);

    // Undo/redo hooks — called only by QUndoCommand subclasses.
    void applyFlagsForUndo(vlUInt flags);
    void applyMinorVersionForUndo(int minor);
    void applyStartFrameForUndo(unsigned int startFrame);
    void applyBumpScaleForUndo(float scale);
    void applyReflectivityForUndo(float x, float y, float z);

    enum class ChannelMode { RGBA, RGB, R, G, B, A, LitNormal, MipDiff };
    enum class BackgroundMode { Checker, Black, White, Custom };
    enum class HdrTonemap { VTFLib, Reinhard, ACES, Clamp };

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void openFileDialog();
    void openVmtDialog();
    void openRecentFile();
    void reloadVtf();
    void openContainingFolder();
    void closeCurrentVtf();
    void closeCurrentVmt();
    void newVmtDialog();
    void createVmtFromCurrentVtf();
    void openMatchingVmtFromVtf();
    void openLinkedVtfFromVmt();
    void formatVmt();
    void liveValidateVmtToggled(bool enabled);
    void liveValidateVmtTick();
    void vtfPropertiesDialog();
    void vtfResourcesDialog();
    void vtfSheetDialog();
    void editVtfFlagsDialog();
    void saveVtf();
    void saveVmt();
    void saveVmtAs();
    void validateVmtStrict();
    void validateVmtLoose();
    void batchConvertDialog();
    void wadConvertDialog();
    void generateNormalMapCurrentFrame();
    void generateNormalMapsAllFrames();
    void generateSphereMap();
    void exportPngDialog();
    void exportThumbnailPngDialog();
    void exportImageAsDialog();
    void copyViewToClipboard();
    void pasteImageFromClipboard();
    void exportAllFramesDialog();
    void exportAllFacesDialog();
    void exportAllSlicesDialog();
    void exportAllMipmapsDialog();
    void exportAllDialog();
    void importImageDialog();
    void saveAsVtfDialog();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void zoomFit();
    void zoomComboChanged();
    void selectionChanged();
    void togglePlayback();
    void playbackTick();
    void cubemapPreviewToggled(bool enabled);
    void findInVmt();
    void findNextInVmt();
    void replaceInVmt();
    void goToLineInVmt();
    void liveSourceReloadToggled(bool enabled);
    void onWatchedPathChanged(const QString &path);
    void watchDebounceTick();

private:
    static constexpr double kMinZoom = 0.01;
    static constexpr double kMaxZoom = 64.0;
    static constexpr std::uint32_t kInvalidImageId = std::numeric_limits<std::uint32_t>::max();

    enum class RecentFileKind { Vtf, Vmt };
    struct RecentFileEntry {
        RecentFileKind kind{};
        QString path;
    };
    static constexpr int kDefaultRecentMax = 8;
    static constexpr int kMaxRecentMax = 32;

    bool importImageFromPath(const QString &path);
    bool importImagesFromPaths(const QStringList &paths);

    void setWatchedVtfPath(const QString &path);
    void clearWatchedVtfPath();
    void addLiveSourcePaths(const QStringList &sources);
    void clearLiveSources();
    void rebuildFileWatcher();
    bool rebuildVtfFromLiveSources();
    void snapshotVtfMtime();
    void scheduleWatchDebounce();
    bool openVmtFromPath(const QString &path);
    bool createVtfFromRgbaImage(const QImage &rgba8888, const QString &suggestedBaseName);
    bool createVtfFromRgbaImages(const QList<QImage> &rgba8888Images,
                                 const QString &suggestedBaseName,
                                 const QStringList &sourcePaths = {});
    bool maybeAutoCreateVmt(const QString &vtfPath);
    QString defaultVmtTextForVtfPath(const QString &vtfPath) const;

    bool warningPopupsEnabled() const;
    bool notificationSoundsEnabled() const;
    bool autoCreateVmtEnabled() const;
    void showInfoPopup(const QString &title, const QString &message);
    void showWarningPopup(const QString &title, const QString &message);
    void showErrorPopup(const QString &title, const QString &message);

    QSize displayedImageSize() const;
    void updateBackgroundComboCustomIcon();
    void selectCustomBackgroundColor();
    void setPreviewPlaceholderText(const QString &text);

    void addRecentVtf(const QString &path);
    void addRecentVmt(const QString &path);
    void rebuildRecentMenus();
    void setRecentFilesMax(int maxFiles);
    void loadUiState();
    void saveUiState() const;

    void addRecentFile(RecentFileKind kind, const QString &path);

    void updateUiFromBoundVtf();
    void updateSelectionLimits();
    void renderSelection(bool showErrorPopup = true);
    void setViewImage(const QImage &rawRgba);
    void rebuildResourcesTree();
    void updateCubemapPreview();
    void setZoom(double zoom, const QPoint &viewportAnchor);
    void setZoom(double zoom);
    void updateZoomUi();
    void updateFitZoom();
    void updateWindowTitle();
    void updateVmtStatusText();
    void setStatusError(const QString &message);
    void setStatusInfo(const QString &message);
    void schedulePreviewRefresh();
    void previewRefreshAttempt(std::uint64_t token, int attempt);

    void closeVtf();
    void closeVmt();
    bool maybeSaveVtf();
    bool maybeSaveVmt();
    void setVmtText(const QString &text, bool markModified);
    QString vmtText() const;
    QString formatVmtText(const QString &text) const;

    std::uint32_t imageId_ = kInvalidImageId;
    QString currentPath_;
    QImage rawRgbaImage_;
    QImage viewImage_;
    bool vtfDirty_ = false;
    double zoom_ = 1.0;
    double prevManualZoom_ = 1.0;
    bool fitToWindow_ = false;
    bool showThumbnail_ = false;
    double hdrExposure_ = 20.0;
    bool hdrTonemapEnabled_ = true;
    HdrTonemap hdrTonemap_ = HdrTonemap::VTFLib;
    bool previewIsHdrTonemapped_ = false;
    std::uint64_t previewRefreshToken_ = 0;
    unsigned int frame_ = 0;
    unsigned int face_ = 0;
    unsigned int slice_ = 0;
    unsigned int mip_ = 0;
    ChannelMode channel_ = ChannelMode::RGBA;
    BackgroundMode background_ = BackgroundMode::Checker;
    QColor customBackgroundColor_ = Qt::white;
    bool maskMode_ = false;
    bool tileMode_ = false;

    QAction *actionOpen_ = nullptr;
    QAction *actionOpenVmt_ = nullptr;
    QAction *actionReloadVtf_ = nullptr;
    QAction *actionDiscardReload_ = nullptr;
    QAction *actionOpenContainingFolder_ = nullptr;
    QAction *actionCloseVtf_ = nullptr;
    QAction *actionCloseVmt_ = nullptr;
    QAction *actionOpenMatchingVmt_ = nullptr;
    QAction *actionOpenLinkedVtf_ = nullptr;
    QAction *actionVtfProperties_ = nullptr;
    QAction *actionVtfResources_ = nullptr;
    QAction *actionEditVtfFlags_ = nullptr;
    QAction *actionSaveVtf_ = nullptr;
    QAction *actionImportImage_ = nullptr;
    QAction *actionSaveAsVtf_ = nullptr;
    QAction *actionExportPng_ = nullptr;
    QAction *actionExportThumbnailPng_ = nullptr;
    QAction *actionExportAs_ = nullptr;
    QAction *actionCopyImage_ = nullptr;
    QAction *actionPasteImage_ = nullptr;
    QAction *actionExportAllFrames_ = nullptr;
    QAction *actionExportAllFaces_ = nullptr;
    QAction *actionExportAllSlices_ = nullptr;
    QAction *actionExportAllMipmaps_ = nullptr;
    QAction *actionExportAll_ = nullptr;
    QAction *actionQuit_ = nullptr;
    QAction *actionBatchConvert_ = nullptr;
    QAction *actionWadConvert_ = nullptr;
    QAction *actionGenNormalFrame_ = nullptr;
    QAction *actionGenNormalAll_ = nullptr;
    QAction *actionGenSphereMap_ = nullptr;
    QAction *actionZoomIn_ = nullptr;
    QAction *actionZoomOut_ = nullptr;
    QAction *actionZoomReset_ = nullptr;
    QAction *actionZoomFit_ = nullptr;
    QAction *actionShowThumbnail_ = nullptr;
    QAction *actionVmtHighlight_ = nullptr;
    QAction *actionCubemapPreview_ = nullptr;
    QAction *actionHdrTonemap_ = nullptr;
    QAction *actionPlayPause_ = nullptr;
    QAction *actionClearRecentFiles_ = nullptr;
    QAction *actionSetRecentLimit_ = nullptr;
    QAction *actionMask_ = nullptr;
    QAction *actionTile_ = nullptr;

    QLabel *imageLabel_ = nullptr;
    QScrollArea *scrollArea_ = nullptr;
    QSpinBox *frameSpin_ = nullptr;
    QSpinBox *faceSpin_ = nullptr;
    QSpinBox *sliceSpin_ = nullptr;
    QSpinBox *mipSpin_ = nullptr;
    QComboBox *channelCombo_ = nullptr;
    QComboBox *backgroundCombo_ = nullptr;
    QSpinBox *frameMsSpin_ = nullptr;
    QComboBox *zoomCombo_ = nullptr;
    QTimer *playbackTimer_ = nullptr;
    bool isPlaying_ = false;

    bool isPanning_ = false;
    QPoint panStartViewportPos_;
    QPoint panStartScroll_;

    QLabel *infoPath_ = nullptr;
    QLabel *infoVersion_ = nullptr;
    QLabel *infoDimensions_ = nullptr;
    QLabel *infoFormat_ = nullptr;
    QLabel *infoMipmaps_ = nullptr;
    QLabel *infoFrames_ = nullptr;
    QLabel *infoFaces_ = nullptr;
    QLabel *infoFlags_ = nullptr;
    QLabel *infoStartFrame_ = nullptr;
    QLabel *infoBumpScale_ = nullptr;
    QLabel *infoReflectivity_ = nullptr;

    QDockWidget *resourcesDock_ = nullptr;
    QTreeWidget *resourcesTree_ = nullptr;

    QDockWidget *cubemapDock_ = nullptr;
    QToolButton *cubeFaceButtons_[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    QButtonGroup *cubeFaceGroup_ = nullptr;
    QSlider *cubemapExposure_ = nullptr;
    double cubemapHdrExposure_ = 20.0;

    QLabel *statusZoom_ = nullptr;
    QLabel *statusPixel_ = nullptr;
    QLabel *statusSelection_ = nullptr;

    QPlainTextEdit *vmtEditor_ = nullptr;
    VmtHighlighter *vmtHighlighter_ = nullptr;
    QString currentVmtPath_;
    QString vmtFindText_;
    QTextDocument::FindFlags vmtFindFlags_ = {};
    QAction *actionSaveVmt_ = nullptr;
    QAction *actionSaveVmtAs_ = nullptr;
    QAction *actionValidateVmtStrict_ = nullptr;
    QAction *actionValidateVmtLoose_ = nullptr;
    QAction *actionNewVmt_ = nullptr;
    QAction *actionCreateVmtFromVtf_ = nullptr;
    QAction *actionFormatVmt_ = nullptr;
    QAction *actionLiveValidateVmt_ = nullptr;

    QMenu *recentFilesMenu_ = nullptr;
    QList<RecentFileEntry> recentFiles_;
    int recentMax_ = kDefaultRecentMax;

    QTimer *vmtValidateTimer_ = nullptr;
    bool liveValidateVmt_ = false;

    QSlider *exposureSlider_ = nullptr;
    QComboBox *hdrTonemapCombo_ = nullptr;

    QLabel *statusVmt_ = nullptr;

    QFileSystemWatcher *fileWatcher_ = nullptr;
    QTimer *watchDebounceTimer_ = nullptr;
    QAction *actionLiveSourceReload_ = nullptr;
    QAction *actionRetuneLiveSources_ = nullptr;
    QLabel *statusLive_ = nullptr;
    Toast *toast_ = nullptr;
    QUndoStack *undoStack_ = nullptr;
    QAction *actionUndo_ = nullptr;
    QAction *actionRedo_ = nullptr;

    QString watchedVtfPath_;
    QString watchedVtfDir_;
    QDateTime watchedVtfMtime_;
    QStringList liveSourcePaths_;
    QList<QDateTime> liveSourceMtimes_;
    SVTFCreateOptions liveCreateOpts_{};
    int liveTextureType_ = 0;
    bool liveUseAlphaFormat_ = false;
    VTFImageFormat liveAlphaFormat_ = IMAGE_FORMAT_DXT5;
    bool liveSourceReloadEnabled_ = false;
    QDateTime selfSaveTimestamp_;
    bool liveRebuildInProgress_ = false;
};
