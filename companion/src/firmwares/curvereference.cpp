/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "curvereference.h"
#include "helpers.h"
#include "modeldata.h"
#include "generalsettings.h"
#include "compounditemmodels.h"
#include "curveimagewidget.h"
#include "curvedialog.h"
#include "sourcenumref.h"
#include "radiodataconversionstate.h"

constexpr char AIM_CRVREF_TYPE[]  {"curvereference.type"};

CurveReference::CurveReference()
{
  clear();
}

void CurveReference::clear()
{
  type = CURVE_REF_DIFF;
  rawSource = RawSource();
}

const QString CurveReference::toString(const ModelData * model, bool verbose, const GeneralSettings * const generalSettings,
                                       Board::Type board, bool prefixCustomName) const
{
  Q_UNUSED(verbose);
  return rawSource.toString(model, generalSettings, board, prefixCustomName);
}

//  static
CurveReference CurveReference::getDefaultValue(const CurveRefType type)
{
  CurveReference cr;
  cr.type = type;
  if (type == CURVE_REF_FUNC)
    cr.rawSource = RawSource(SOURCE_TYPE_CURVE_FUNC, 1);
  else if (type == CURVE_REF_CUSTOM)
    cr.rawSource = RawSource(SOURCE_TYPE_CURVE, 1);
  else
    cr.rawSource = RawSource();

  return cr;
}

//  static
QString CurveReference::typeToString(const int type)
{
  const QStringList strl = { tr("Diff"), tr("Expo") , tr("Func"), tr("Custom") };
  int idx = (int)type;

  if (idx < 0 || idx > MAX_CURVE_REF_COUNT)
    return CPN_STR_UNKNOWN_ITEM;

  return strl.at(idx);
}

//  static
AbstractItemModel *CurveReference::typeItemModel()
{
   AbstractStaticItemModel * mdl = new AbstractStaticItemModel();
   mdl->setName(AIM_CRVREF_TYPE);

   for (int i = 0; i < MAX_CURVE_REF_COUNT; i++) {
     mdl->appendToItemList(typeToString(i), i);
   }

   mdl->loadItemList();
   return mdl;
}

CurveReference CurveReference::convert(RadioDataConversionState & cstate)
{
  rawSource = rawSource.convert(cstate);
}

/*
 * CurveReferenceUIManager
*/

CurveReferenceUIManager::CurveReferenceUIManager(QComboBox * cboType, QCheckBox * chkUseSource, QSpinBox * sbxValue,
                                                 QComboBox * cboSource, CurveImageWidget * curveImage,
                                                 CurveReference & curveRef, ModelData & model, CompoundItemModelFactory * sharedItemModels,
                                                 QObject * parent) :
  QObject(parent),
  cboType(cboType),
  curveImage(curveImage),
  curveRef(curveRef),
  lock(false)
{
  sourceNumRefUIEditor = new SourceNumRefUIEditor(curveRef.rawSource, chkUseSource, sbxValue, cboSource, 0, -100, 100, 1, model, sharedItemModels);

  if (cboType) {
    cboType->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    cboType->setModel(CurveReference::typeItemModel());
    cboType->setCurrentIndex(cboType->findData((int)curveRef.type));
    connect(cboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CurveReferenceUIManager::cboTypeChanged);
  }

  if (curveImage) {
    curveImage->set(&model, getCurrentFirmware(), sharedItemModels, curveRef.rawSource.index, Qt::black, 3);
    curveImage->setGrid(Qt::gray, 2);
    connect(curveImage, &CurveImageWidget::doubleClicked, this, &CurveReferenceUIManager::curveImageDoubleClicked);
  }

  update();
}

CurveReferenceUIManager::~CurveReferenceUIManager()
{
  delete sourceNumRefUIEditor;
}

void CurveReferenceUIManager::update()
{
  lock = true;

  sourceNumRefUIEditor->setLock(true);

  int widgetsMask = 0;

  if (cboType)
    cboType->setCurrentIndex(cboType->findData(curveRef.type));

  if (curveRef.type == CurveReference::CURVE_REF_DIFF || curveRef.type == CurveReference::CURVE_REF_EXPO) {
    sourceNumRefUIEditor->setVisible(true);
    sourceNumRefUIEditor->update();
  } else {
    sourceNumRefUIEditor->setVisible(false);
  }

  if (curveImage) {
    if (curveRef.type == CurveReference::CURVE_REF_CUSTOM &&
        curveRef.rawSource.type == RawSourceType::SOURCE_TYPE_CURVE) {
      curveImage->setIndex(curveRef.rawSource.index);

      if (abs(curveRef.rawSource.index) > 0 && abs(curveRef.rawSource.index) <= CPN_MAX_CURVES)
        curveImage->setPen(colors[abs(curveRef.rawSource.index) - 1], 3);
      else
        curveImage->setPen(Qt::black, 3);

      curveImage->draw();
      curveImage->setVisible(true);
    } else {
      curveImage->setVisible(false);
    }
  }

  emit resized();

  sourceNumRefUIEditor->setLock(false);

  lock = false;
}

void CurveReferenceUIManager::cboTypeChanged(int index)
{
  if (!lock) {
    CurveReference::CurveRefType type = (CurveReference::CurveRefType)cboType->itemData(index).toInt();
    curveRef = CurveReference::getDefaultValue(type);
    update();
  }
}

void CurveReferenceUIManager::curveImageDoubleClicked()
{
  if (curveRef.type == CurveReference::CURVE_REF_CUSTOM && abs(curveRef.rawSource.index) > 0)
    curveImage->edit();
}
