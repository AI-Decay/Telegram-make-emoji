#include "InputSliderWidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox >
#include <QString>
#include <QVBoxLayout>

#include "utility/StyleSheetUtility.h"

namespace {
constexpr QSize minSliderSize(100, 30);
constexpr auto InputSliderWidgetStyle = ":/styles/InputSliderWidgetStyle.css";
constexpr auto LabelMaxText = "End position in ms";
constexpr auto LabelMinText = "Begin position in ms";
constexpr auto SpinBoxMaxObjectName = "SpinBoxMax";
constexpr auto SpinBoxMinObjectName = "SpinBoxMin";
constexpr auto LabelMaxObjectName = "LabelMax";
constexpr auto LabelMinObjectName = "LabelMin";
}  // namespace

InputSliderWidget::InputSliderWidget(QWidget* parent) : QWidget{parent} {
  setStyleSheet(loadStyleSheet(InputSliderWidgetStyle));

  auto* mainLayout = new QHBoxLayout(this);
  auto* sliderLayout = new QVBoxLayout();
  auto* maxLayout = new QVBoxLayout();
  auto* minLayout = new QVBoxLayout();

  maxLayout->setSpacing(0);
  minLayout->setSpacing(0);

  spinBoxMax = createWidget<QSpinBox>(this, SpinBoxMaxObjectName);
  spinBoxMin = createWidget<QSpinBox>(this, SpinBoxMinObjectName);
  auto* labelMax = createWidget<QLabel>(this, LabelMaxObjectName);
  auto* labelMin = createWidget<QLabel>(this, LabelMinObjectName);

  labelMax->setText(LabelMaxText);
  labelMin->setText(LabelMinText);
  spinBoxMax->setMinimumSize(minSliderSize);
  spinBoxMin->setMinimumSize(minSliderSize);
  spinBoxMax->setMinimum(0);
  spinBoxMin->setMinimum(0);

  maxLayout->addWidget(labelMax);
  maxLayout->addWidget(spinBoxMax);

  minLayout->addWidget(labelMin);
  minLayout->addWidget(spinBoxMin);

  sliderLayout->addLayout(maxLayout);
  sliderLayout->addLayout(minLayout);

  mainLayout->addLayout(sliderLayout);
}

void InputSliderWidget::setLimit(int max) {
  spinBoxMax->setMaximum(max);
  spinBoxMin->setMaximum(max);
}

void InputSliderWidget::setBegin(int value) { spinBoxMin->setValue(value); }
void InputSliderWidget::setEnd(int value) { spinBoxMax->setValue(value); }

int InputSliderWidget::getBegin() const { return spinBoxMin->value(); }
int InputSliderWidget::getEnd() const { return spinBoxMax->value(); }
