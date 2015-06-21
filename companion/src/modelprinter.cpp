
#include "helpers.h"
#include "modelprinter.h"

QString changeColor(const QString & input, const QString & to, const QString & from)
{
  QString result = input;
  return result.replace("color="+from, "color="+to);
}

ModelPrinter::ModelPrinter(FirmwareInterface * firmware,  
                            GeneralSettings *gg, ModelData *gm) :
  firmware(firmware),
  g_eeGeneral(gg),
  g_model(gm)
{
}  

void debugHtml(const QString & html)
{
  QFile file("foo.html");
  file.open(QIODevice::Truncate | QIODevice::WriteOnly);
  file.write(html.toUtf8());
  file.close();
}

QString addFont(const QString & input, const QString & color, const QString & size, const QString & face)
{
  QString colorStr;
  if (!color.isEmpty()) {
    colorStr = "color=" + color;
  }
  QString sizeStr;
  if (!size.isEmpty()) {
    sizeStr = "size=" + size;
  }
  QString faceStr;
  if (!face.isEmpty()) {
    faceStr = "face='" + face + "'";
  }
  return "<font " + sizeStr + " " + faceStr + " " + colorStr + ">" + input + "</font>";  
}

QString  ModelPrinter::printInputName(int idx)
{
  QString result;
  if (GetCurrentFirmware()->getCapability(VirtualInputs)) {
    if (strlen(g_model->inputNames[idx]) > 0) {
      result = QObject::tr("[I%1]").arg(idx+1);
      result += QString(g_model->inputNames[idx]);
    }
    else {
      result = QObject::tr("Input%1").arg(idx+1, 2, 10, QChar('0'));
    }
  }
  else {
    result = RawSource(SOURCE_TYPE_STICK, idx).toString(*g_model);
  }
  return Qt::escape(result);
}

QString ModelPrinter::printInputLine(const ExpoData * ed)
{
  QString str = "&nbsp;";

  switch (ed->mode) {
    case (1): str += "&lt;-"; break;
    case (2): str += "-&gt;"; break;
    default:  str += "&nbsp;&nbsp;"; break;
  }

  str += "&nbsp;" + Qt::escape(QObject::tr("Weight")) + QString("(%1)").arg(getGVarString(ed->weight,true));

  if (firmware->getCapability(VirtualInputs)) {
    str += " " + Qt::escape(QObject::tr("Source") + QString("(%1)").arg(ed->srcRaw.toString(*g_model)));
    if (ed->carryTrim>0) str += " " + Qt::escape(QObject::tr("NoTrim"));
    else if (ed->carryTrim<0) str += " " + Qt::escape(RawSource(SOURCE_TYPE_TRIM, (-(ed->carryTrim)-1)).toString(*g_model));
  }
  if (ed->curve.value) str += " " + Qt::escape(ed->curve.toString());

  QString phasesStr = getPhasesStr(ed->phases);
  if (!phasesStr.isEmpty()) str += " " + Qt::escape(phasesStr);

  if (ed->swtch.type != SWITCH_TYPE_NONE) 
    str += " " + Qt::escape(QObject::tr("Switch") + QString("(%1)").arg(ed->swtch.toString()));
  if (firmware->getCapability(HasExpoNames) && ed->name[0]) 
    str += Qt::escape(QString(" [%1]").arg(ed->name));
  // str += "</font>";
  return str;
}

QString ModelPrinter::printMixerName(int curDest)
{
  QString str;
  str = QObject::tr("CH%1").arg(curDest);
  // TODO not nice, Qt brings a function for that, I don't remember right now
  (str.length() < 4) ? str.append("  ") : str.append(" ");
  if (firmware->getCapability(HasChNames)) {
    QString name = g_model->limitData[curDest-1].name;
    if (!name.isEmpty()) {
      name = QString("(") + name + QString(")");
    }
    name.append("        ");
    str += name.left(8);
  }
  return Qt::escape(str);
}

QString ModelPrinter::printMixerLine(const MixData * md, int highlightedSource)
{
  QString str = "&nbsp;";

  switch(md->mltpx) {
    case (1): str += "*"; break;
    case (2): str += "R"; break;
    default:  str += "&nbsp;"; break;
  };

  // highlight source if needed
  QString source = Qt::escape(md->srcRaw.toString(*g_model));
  if ( (md->srcRaw.type == SOURCE_TYPE_CH) && (md->srcRaw.index+1 == (int)highlightedSource) ) {
    source = "<b>" + source + "</b>";
  }
  str += "&nbsp;" + source;

  str += " " + Qt::escape(QObject::tr("Weight")) + QString("(%1)").arg(getGVarString(md->weight, true));

  QString phasesStr = getPhasesStr(md->phases);
  if (!phasesStr.isEmpty()) str += " " + Qt::escape(phasesStr);

  if (md->swtch.type != SWITCH_TYPE_NONE) {
    str += " " + Qt::escape(QObject::tr("Switch")) + QString("(%1)").arg(md->swtch.toString());
  }

  if (md->carryTrim>0)      str += " " + Qt::escape(QObject::tr("NoTrim"));
  else if (md->carryTrim<0) str += " " + RawSource(SOURCE_TYPE_TRIM, (-(md->carryTrim)-1)).toString(*g_model);

  if (firmware->getCapability(HasNoExpo) && md->noExpo) str += " " + Qt::escape(QObject::tr("No DR/Expo"));
  if (md->sOffset)     str += " " + Qt::escape(QObject::tr("Offset")) + QString("(%1)").arg(getGVarString(md->sOffset));
  if (md->curve.value) str += " " + Qt::escape(md->curve.toString());

  int scale = firmware->getCapability(SlowScale);
  if (scale == 0)
    scale = 1;
  if (md->delayDown || md->delayUp)
    str += " " + Qt::escape(QObject::tr("Delay")) + QString("(u%1:d%2)").arg((double)md->delayUp/scale).arg((double)md->delayDown/scale);
  if (md->speedDown || md->speedUp)
    str += " " + Qt::escape(QObject::tr("Slow")) + QString("(u%1:d%2)").arg((double)md->speedUp/scale).arg((double)md->speedDown/scale);
  if (md->mixWarn)
    // str += Qt::escape(QObject::tr(" Warn(%1)").arg(md->mixWarn));
    str += " " + Qt::escape(QObject::tr("Warn")) + QString("(%1)").arg(md->mixWarn);
  if (firmware->getCapability(HasMixerNames) && md->name[0]) 
    str += Qt::escape(QString(" [%1]").arg(md->name));
  return str;
}

QString ModelPrinter::getPhasesStr(unsigned int phases)
{
  int numphases = GetCurrentFirmware()->getCapability(FlightModes);

  if (numphases && phases) {
    QString str;
    int count = 0;
    if (phases == (unsigned int)(1<<numphases) - 1) {
      str = " ### " + QObject::tr("DISABLED") + " ### ";
    }
    if (phases) {
      for (int i=0; i<numphases;i++) {
        if (!(phases & (1<<i))) {
          if (count++ > 0) str += QString(", ");
          str += getPhaseName(i+1, g_model->flightModeData[i].name);
        }
      }
    }
    if (count > 1)
      return QObject::tr("Flight modes") + QString("(%1)").arg(str);
    else
      return QObject::tr("Flight mode") + QString("(%1)").arg(str);
  }
  return "";
}

