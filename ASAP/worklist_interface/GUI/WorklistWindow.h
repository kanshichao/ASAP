#ifndef __ASAP_GUI_WORKLISTDATAACQUISITIONINTERFACE__
#define __ASAP_GUI_WORKLISTDATAACQUISITIONINTERFACE__

#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <QDropEvent>
#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>
#include <QFutureWatcher>
#include <QSettings>

#include "CompositeChild.h"
#include "ui_WorklistWindowLayout.h"
#include "ASAP/ASAP_Window.h"
#include "../Data/SourceProxy.h"
#include "../Misc/TemporaryDirectoryTracker.h"
#include "../Models/WorklistModels.h"

namespace ASAP
{
	// TODO: Decouple source management from worklist window
	class WorklistWindow : public CompositeChild
	{
		Q_OBJECT

		public:
			explicit WorklistWindow(QWidget* parent = nullptr);
			~WorklistWindow(void);

			void AttachWorkstation(ASAP_Window& workstation, const int tab_id);

			void SetDataSource(const std::string source_path, const std::unordered_map<std::string, std::string> parameters);
			void SetDataSource(const std::string& source);

		public slots:
			void UpdateImageIcons(int itemRow, const QIcon& newIcon);
			void UpdateStatusBar(const QString& message);

		signals:
			void ShowMessageBox(const QString message);
			void RequestStatusBarUpdate(const QString& message);
			void RequestOpenImage(const QString& filepath);
			void RequestWorklistRefresh(void);

		private:
			SourceProxy									m_source_;
			std::unique_ptr<Ui::WorklistWindowLayout>	m_ui_;
			std::unique_ptr<QFutureWatcher<void> >      m_thumbnail_loader;
			std::mutex									m_image_switch_access_;
			std::mutex									m_status_bar_access_;
			TemporaryDirectoryTracker					m_storage_directory_;
			std::vector<std::unique_ptr<QAction>>		m_history_actions_;
			ASAP_Window*								m_workstation_; // Todo: Clean up or perhaps combine in struct
			int											m_workstation_tab_id_;
			WorklistModels								m_models_;
			QSettings*									m_settings;

			bool CheckSchema_(void);
			void LoadSettings_(void);
			void StoreSettings_(void);
			void StopThumbnailLoading_(void);
			void UpdatePreviousSources_(void);
			void UpdateSourceViews_(void);

			std::vector<std::string> GetImagesForItem_(const std::string& id, const WorklistModels::ModelEnum model);
			void WorklistWindow::DeleteFromWorklist_(QStandardItem* item, const WorklistModels::ModelEnum model);
			void UpdateWorklist_(const QStandardItem* worklist_item, const std::vector<std::string>& image_list, bool remove);

			void SetModels_(void);
			void SetSlots_(void);

			void keyPressEvent(QKeyEvent* event);
			bool eventFilter(QObject* obj, QEvent* event);
			void GetImageFromIndex(const QModelIndex& index);


		private slots:
			void OnWorklistClear_(QModelIndex index, int row, int column);
			void OnPatientsClear_(QModelIndex index, int row, int column);
			void OnStudyClear_(QModelIndex index, int row, int column);

			void OnWorklistSelect_(QModelIndex index);
			void OnPatientSelect_(QModelIndex index);
			void OnStudySelect_(QModelIndex index);
			void OnImageSelect_(const bool checked);
			void OnSelectFileSource_(bool checked);
			void OnSelectFolderSource_(bool checked);
			void OnSelectExternalSource_(bool checked);

		//	void OnDeletePress_();
			void OnShowMessageBox_(const QString message);
			void OnImageDrop_(QDropEvent* drop_event);
			void OnOpenImage_(QString path);
			void OnIconDoubleClicked(const QModelIndex& index);
			void OnCreateWorklist_(void);
			void OnWorklistNameChange_(QStandardItem* item);
			void OnWorklistRefresh_(void);
	};
}
#endif // __ASAP_GUI_WORKLISTDATAACQUISITIONINTERFACE__