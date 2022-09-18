#ifndef INPUTSLIDERWIDGET_H
#define INPUTSLIDERWIDGET_H

#include <QWidget>

class QSpinBox;

class InputSliderWidget : public QWidget {
  Q_OBJECT
 public:
  explicit InputSliderWidget(QWidget* parent = nullptr);
  void setLimit(int max);
  void setBegin(int value);
  void setEnd(int value);
  int getBegin() const;
  int getEnd() const;

 private:
  QSpinBox* spinBoxMax = nullptr;
  QSpinBox* spinBoxMin = nullptr;
};

#endif  // INPUTSLIDERWIDGET_H
