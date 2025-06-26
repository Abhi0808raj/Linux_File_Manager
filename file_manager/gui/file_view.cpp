#include "gui/file_view.hpp"

FileView::FileView(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("FileView placeholder"));
    setLayout(layout);
}

FileView::~FileView() = default;
