#include "WorklistWindow.h"

#include <cctype>
#include <unordered_map>

#include <boost/bind.hpp>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <QtConcurrent\qtconcurrentrun.h>

#include "IconCreation.h"
#include "INI_Parsing.h"
#include "Data/SourceLoading.h"

namespace ASAP::Worklist::GUI
{
	WorklistWindow::WorklistWindow(QWidget *parent) :
		QMainWindow(parent),
		m_ui_(new Ui::WorklistWindowLayout),
		m_data_acquisition_(nullptr),
		m_images_model_(new QStandardItemModel(0, 0)),
		m_patients_model_(new QStandardItemModel(0, 0)),
		m_studies_model_(new QStandardItemModel(0, 0)),
		m_worklist_model_(new QStandardItemModel(0, 0))
	{
		m_ui_->setupUi(this);
		SetSlots_();
		SetModels_();
		LoadSettings_();
		SetDataSource(m_settings_.source_location);
	}

	void WorklistWindow::AttachWorkstation(PathologyWorkstation& workstation)
	{
		m_workstation_ = &workstation;
	}

	WorklistWindowSettings WorklistWindow::GetStandardSettings(void)
	{
		return WorklistWindowSettings();
	}

	void WorklistWindow::SetDataSource(const std::string source_path)
	{
		try
		{
			m_data_acquisition_ = Data::LoadDataSource(source_path);
			AdjustGuiToSource_();
		}
		catch (const std::exception& e)
		{
			QMessageBox::question(this, "Error", e.what(), QMessageBox::Ok);
		}
	}

	void WorklistWindow::SetWorklistItems(const DataTable& items, QStandardItemModel* model)
	{
		std::unordered_map<std::string, QStandardItem*> inserted_items;
		model->removeRows(0, model->rowCount());
		for (size_t item = 0; item < items.Size(); ++item)
		{
			std::vector<const std::string*> record(items.At(item, std::vector<std::string>{"id", "title", "parent"}));
			QStandardItem* model_item(new QStandardItem(QString(record[1]->data())));

			model_item->setData(QVariant(std::stoi(*record[0])));
			inserted_items.insert({ *record[0], model_item });

			if (record[2]->empty())
			{
				model->setItem(m_worklist_model_->rowCount(), 0, model_item);
			}
			else
			{
				auto parent = inserted_items.find(*record[2]);
				if (parent != inserted_items.end())
				{
					parent->second->appendRow(model_item);
				}
			}
		}
	}

	void WorklistWindow::SetPatientsItems(const DataTable& items, QStandardItemModel* model)
	{
		model->removeRows(0, model->rowCount());
		model->setRowCount(items.Size());
		for (size_t item = 0; item < items.Size(); ++item)
		{
			std::vector<const std::string*> record(items.At(item, DataTable::FIELD_SELECTION::VISIBLE));
			size_t record_id = std::stoi(*record[0]);

			for (size_t field = 0; field < record.size(); ++field)
			{
				QStandardItem* model_item = new QStandardItem(QString(record[field]->data()));
				model_item->setData(QVariant(record_id));
				model->setItem(item, field, model_item);
			}
		}
	}

	void WorklistWindow::SetStudyItems(const DataTable& items, QStandardItemModel* model)
	{
		model->removeRows(0, model->rowCount());
		model->setRowCount(items.Size());
		for (size_t item = 0; item < items.Size(); ++item)
		{
			std::vector<const std::string*> record(items.At(item, DataTable::FIELD_SELECTION::VISIBLE));
			size_t record_id = std::stoi(*record[0]);

			for (size_t field = 0; field < record.size(); ++field)
			{
				QStandardItem* model_item = new QStandardItem(QString(record[field]->data()));
				model_item->setData(QVariant(record_id));
				model->setItem(item, field, model_item);
			}
		}
	}

	void WorklistWindow::SetImageItems(const DataTable& items, QStandardItemModel* model)
	{
		model->removeRows(0, model->rowCount());
		model->setRowCount(items.Size());

		std::vector<std::pair<QStandardItem*, std::string>> icon_items;
		for (size_t item = 0; item < items.Size(); ++item)
		{
			std::vector<const std::string*> record(items.At(item, { "id", "location", "title" }));

			QStandardItem* model_item = new QStandardItem(QString(record[2]->data()));
			model_item->setData(QVariant(QString(record[1]->data())));
			model->setItem(item, 0, model_item);

			icon_items.push_back({ model_item, *record[1] });
		}

		QtConcurrent::run([icon_items, size=500]()
		{
			CreateIcons(icon_items, size);
		});
	}

	void WorklistWindow::AdjustGuiToSource_(void)
	{
		Data::WorklistDataAcquisitionInterface::SourceType type = Data::WorklistDataAcquisitionInterface::SourceType::FILELIST;

		if (m_data_acquisition_)
		{
			type = m_data_acquisition_->GetSourceType();

			if (type == Data::WorklistDataAcquisitionInterface::SourceType::FILELIST)
			{
				QStandardItemModel* image_model = m_images_model_;
				m_data_acquisition_->GetImageRecords(0, [this, image_model](DataTable& table, int error)
				{
					if (error == 0)
					{
						this->SetImageItems(table, image_model);
					}
				});
			}
			else if (type == Data::WorklistDataAcquisitionInterface::SourceType::FULL_WORKLIST)
			{
				SetHeaders_(m_data_acquisition_->GetPatientHeaders(), m_patients_model_, m_ui_->view_patients);
				SetHeaders_(m_data_acquisition_->GetStudyHeaders(), m_studies_model_, m_ui_->view_studies);

				QStandardItemModel* worklist_model = m_worklist_model_;
				m_data_acquisition_->GetWorklistRecords([this, worklist_model](DataTable& table, int error)
				{
					if (error == 0)
					{
						this->SetWorklistItems(table, worklist_model);
					}
				});
			}
		}

		// Default is regarded as the FILELIST sourcetype
		switch (type)
		{
			case Data::WorklistDataAcquisitionInterface::SourceType::FULL_WORKLIST: 
				m_ui_->label_worklists->setVisible(true);
				m_ui_->label_patients->setVisible(true);
				m_ui_->label_studies->setVisible(true);
				m_ui_->view_worklists->setVisible(true);
				m_ui_->view_patients->setVisible(true);
				m_ui_->view_studies->setVisible(true);
				break;
			default:
				m_ui_->label_worklists->setVisible(false);
				m_ui_->label_patients->setVisible(false);
				m_ui_->label_studies->setVisible(false);
				m_ui_->view_worklists->setVisible(false);
				m_ui_->view_patients->setVisible(false);
				m_ui_->view_studies->setVisible(false);
				break;
		}
	}

	void WorklistWindow::LoadSettings_(void)
	{
		try
		{
			std::unordered_map<std::string, std::string> values(INI::ParseINI("worklist_config.ini"));
			auto source_value(values.find("source"));
			if (source_value != values.end())
			{
				m_settings_.source_location = source_value->second;
			}
		}
		catch (const std::runtime_error& e)
		{
			// Creates an INI file with standard settings.
			std::unordered_map<std::string, std::string> values;
			values.insert({ "source", "" });
			INI::WriteINI("worklist_config.ini", values);
		}
	}

	void WorklistWindow::SetHeaders_(std::vector<std::string> headers, QStandardItemModel* model, QAbstractItemView* view)
	{
		QStringList q_headers;
		for (std::string& header : headers)
		{
			header[0] = std::toupper(header[0]);
			q_headers.push_back(QString(header.c_str()));
		}
		model->setColumnCount(q_headers.size());
		model->setHorizontalHeaderLabels(q_headers);
		view->update();
	}

	void WorklistWindow::SetModels_(void)
	{
		m_ui_->view_images->setModel(m_images_model_);
		m_ui_->view_patients->setModel(m_patients_model_);
		m_ui_->view_studies->setModel(m_studies_model_);
		m_ui_->view_worklists->setModel(m_worklist_model_);
	}

	//================================= Slots =================================//
	
	void WorklistWindow::SetSlots_(void)
	{
		connect(m_worklist_model_,
			SIGNAL(rowsRemoved(QModelIndex, int, int)),
			this,
			SLOT(OnWorklistClear_(QModelIndex, int, int)));

		connect(m_patients_model_,
			SIGNAL(rowsRemoved(QModelIndex, int, int)),
			this,
			SLOT(OnPatientsClear_(QModelIndex, int, int)));

		connect(m_studies_model_,
			SIGNAL(rowsRemoved(QModelIndex, int, int)),
			this,
			SLOT(OnStudyClear_(QModelIndex, int, int)));

		connect(m_ui_->view_worklists,
			SIGNAL(clicked(QModelIndex)),
			this,
			SLOT(OnWorklistSelect_(QModelIndex)));

		connect(m_ui_->view_patients,
			SIGNAL(clicked(QModelIndex)),
			this,
			SLOT(OnPatientSelect_(QModelIndex)));

		connect(m_ui_->view_studies,
			SIGNAL(clicked(QModelIndex)),
			this,
			SLOT(OnStudySelect_(QModelIndex)));

		connect(m_ui_->view_images,
			SIGNAL(clicked(QModelIndex)),
			this,
			SLOT(OnImageSelect_(QModelIndex)));

		connect(m_ui_->actionOpenSource,
			SIGNAL(triggered),
			this,
			SLOT(OnSelectLocalSource_()));
		connect(m_ui_->actionOpenExternalSource,
			SIGNAL(triggered),
			this,
			SLOT(OnSelectExternalSource_()));
	}

	void WorklistWindow::OnWorklistClear_(QModelIndex index, int, int)
	{
		m_patients_model_->removeRows(0, m_patients_model_->rowCount());
		m_ui_->view_patients->update();
	}

	void WorklistWindow::OnPatientsClear_(QModelIndex index, int, int)
	{
		m_studies_model_->removeRows(0, m_studies_model_->rowCount());
		m_ui_->view_studies->update();
	}

	void WorklistWindow::OnStudyClear_(QModelIndex index, int, int)
	{
		m_images_model_->removeRows(0, m_images_model_->rowCount());
		m_ui_->view_images->update();
	}

	void WorklistWindow::OnWorklistSelect_(QModelIndex index)
	{
		QStandardItem* item(m_worklist_model_->itemFromIndex(index));
		int worklist_id = item->data().toInt();

		QStandardItemModel* patient_model = m_patients_model_;
		QTableView* patient_view = m_ui_->view_patients;
		m_data_acquisition_->GetPatientRecords(worklist_id, [this, patient_model, patient_view](DataTable table, int error)
		{
			if (error == 0)
			{
				this->SetPatientsItems(table, patient_model);
				patient_view->update();
			}
		});
	}

	void WorklistWindow::OnPatientSelect_(QModelIndex index)
	{
		QStandardItem* item(m_patients_model_->itemFromIndex(index));
		int patient_id = item->data().toInt();

		QStandardItemModel* study_model = m_studies_model_;
		QTableView* study_view = m_ui_->view_studies;
		m_data_acquisition_->GetStudyRecords(patient_id, [this, study_model, study_view](DataTable table, int error)
		{
			if (error == 0)
			{
				this->SetStudyItems(table, study_model);
				study_view->update();
			}
		});
	}

	void WorklistWindow::OnStudySelect_(QModelIndex index)
	{
		QStandardItem* item(m_studies_model_->itemFromIndex(index));
		int study_id = item->data().toInt();

		QStandardItemModel* image_model = m_images_model_;
		QListView* image_view = m_ui_->view_images;
		m_data_acquisition_->GetImageRecords(study_id, [this, image_model, image_view](DataTable table, int error)
		{
			if (error == 0)
			{
				this->SetImageItems(table, image_model);
				image_view->update();
			}
		});
	}

	void WorklistWindow::OnImageSelect_(QModelIndex index)
	{
		QStandardItem* item(m_images_model_->itemFromIndex(index));
		QString image_handle = item->data().toString();

		if (m_workstation_)
		{
			m_workstation_->openFile(image_handle);
		}
	}

	void WorklistWindow::OnSelectLocalSource_(void)
	{
		/*QFileDialog* dialog = new QFileDialog(this);
		dialog->setFileMode(QFileDialog::Directory);
		dialog->setOption(QFileDialog::DontUseNativeDialog, true);

		// Try to select multiple files and directories at the same time in QFileDialog
		QListView* listview = dialog->findChild<QListView*>("listView");
		if (listview)
		{
			listview->setSelectionMode(QAbstractItemView::MultiSelection);
		}
		QTreeView* treeview = dialog->findChild<QTreeView*>();
		if (treeview)
		{
			treeview->setSelectionMode(QAbstractItemView::MultiSelection);
		}

		dialog->exec();
		QStringList _fnames = _f_dlg->selectedFiles();

		*/

		QFileDialog* dialog;
		dialog->setProxyModel(nullptr);
		dialog->exec();
		SetDataSource(dialog->selectedFiles()[0].toUtf8().constData());
	}

	void WorklistWindow::OnSelectExternalSource(void)
	{

	}
}