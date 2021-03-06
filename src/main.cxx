#include <iostream>
#include <fstream>
#include <string>

#include <QApplication>
#include <QPushButton>
#include <QString>
#include <QFileInfo>
#include <QTreeWidget>

#include "GMainWindow.h"
// #include "main.h"

#include <stddef.h> /* size_t */
#include <string.h> /* strcmp */
#include <iostream>

#include "IntensityMotionCheck.h"

#include <itksys/SystemTools.hxx>

#include "Protocol.h"
#include "XmlStreamReader.h"
#include "XmlStreamWriter.h"
#include "DTIPrepCLP.h"
#include <sstream>

// Defining Checking bits
#define ImageCheckBit 1
#define DiffusionCheckBit 2
#define SliceWiseCheckBit 4
#define InterlaceWiseCheckBit 8
#define GradientWiseCheckBit 16
#define BrainMaskBit 32
#define DominantDirectionDetectBit 64

// #include <QStyleFactory>
using namespace std;

int r_SliceWiseCkeck;     // the overall SliceWiseChecking result
int r_InterlaceWiseCheck; // the overall InterlaceWiseChecking result
int r_GradWiseCheck;      // the overall GradientWiseChecking result

int main( int argc, char * *argv )
{
  PARSE_ARGS;
  std::ostringstream ssNumberOfThreads ;
  ssNumberOfThreads << "ITK_GLOBAL_DEFAULT_NUMBER_OF_THREADS=" << numberOfThreads ;
  itksys::SystemTools::PutEnv( ssNumberOfThreads.str().c_str() ) ;
// BUG: When loading the DTIPrep GUI, the following error would appear: "Qt internal error: qt_menu.nib could not be
// loaded. The .nib file should be placed in QtGui.framework/Versions/Current/Resources/ or in the resources directory
// of your applicaiton bundle." Qt is aware of this problem (http://bugreports.qt.nokia.com/browse/QTBUG-5952) and has
// provided the following fix that was pasted in main.cxx and got rid of the error.

#if 0
  QT_MANGLE_NAMESPACE(QCocoaMenuLoader) * qtMenuLoader = [[QT_MANGLE_NAMESPACE(QCocoaMenuLoader) alloc] init];
  if([NSBundle loadNibNamed : @ "qt_menu" owner : qtMenuLoader] == false )
    {
    qFatal(
      "Qt internal error: qt_menu.nib could not be loaded. The .nib file"
      " should be placed in QtGui.framework/Versions/Current/Resources/ "
      " or in the resources directory of your application bundle.");
    }

  [cocoaApp setMenu :[qtMenuLoader menu]];
  [newDelegate setMenuLoader : qtMenuLoader];
  [qtMenuLoader release];
#endif
  if( !bcheckByProtocol )
    {
    QApplication app(argc, argv);
    QLocale::setDefault(QLocale::C);
    setlocale(LC_ALL, "C");
    GMainWindow *MainWindow = new GMainWindow;
    MainWindow->show();
    MainWindow->raise();
    const int return_status = app.exec();
    if ( return_status == EXIT_SUCCESS )
        {
        std::cout << "SUCCESSFUL execution of " << argv[0] << std::endl;
        }
    else
        {
        std::cout << "FAILED execution of " << argv[0] << " in " << __FILE__ << " at " << __LINE__ << std::endl;
        }
    return return_status;
    }
  else
    {

    string resultXMLFile;

    // check with  xml
    if( !xmlFileName.empty() )
      {

      Protocol protocol;
      QCResult qcResult;
      qcResult.Clear();
      protocol.clear();

      protocol.GetQCOutputDirectory() = resultFolder;
      std::cout << "GetQCOutputDirectory" << protocol.GetQCOutputDirectory() << std::endl;

      // if ( resultXMLFile.length() <= 0 )
      // {

      std::string Full_path;
      std::string Dwi_file_name;    // Full name of dwi image
      size_t      found2 = DWIFileName.find_last_of(".");
      Full_path = DWIFileName.substr( 0, found2);
      Dwi_file_name = Full_path.substr(Full_path.find_last_of("/\\") + 1);

      if( protocol.GetQCOutputDirectory().length() > 0 )
        {

        std::string str_QCOutputDirectory = protocol.GetQCOutputDirectory();
        size_t      found_SeparateChar = str_QCOutputDirectory.find_first_of("/");
        if( int (found_SeparateChar) == -1 )  // "/" does not exist in the protocol->GetQCOutputDirectory() and
                                              // interpreted as the relative path and creates the folder
          {

          size_t found;
          found = DWIFileName.find_last_of("/\\");
          std::string str;
          str = DWIFileName.substr( 0, found ); // str : path of QCed outputs
          str.append( "/" );
          str.append( protocol.GetQCOutputDirectory() );
          if( !itksys::SystemTools::FileIsDirectory( str.c_str() ) )
            {
            itksys::SystemTools::MakeDirectory( str.c_str() );
            }
          str.append( "/" );
          resultXMLFile = str;
          resultXMLFile.append( Dwi_file_name );
          resultXMLFile.append( "_XMLQCResult.xml" );
          }

        else // "/" exists in the the protocol->GetQCOutputDirectory() and interpreted as the absolute path
          {
          std::string str;
          str.append(protocol.GetQCOutputDirectory() );
          if( !itksys::SystemTools::FileIsDirectory( str.c_str() ) )
            {
            itksys::SystemTools::MakeDirectory( str.c_str() );
            }
          str.append( "/" );
          resultXMLFile = str;
          resultXMLFile.append( Dwi_file_name );
          resultXMLFile.append( "_XMLQCResult.xml" );

          }

        }
      else
        {
        resultXMLFile = DWIFileName.substr( 0, DWIFileName.find_last_of('.') );
        resultXMLFile.append( "_XMLQCResult.xml" );
        }

      // QString Result_xmlFile = QString::fromStdString( DWIFileName ).section('.',-2,0);
      // Result_xmlFile.remove("_QCed");
      // Result_xmlFile.append(QString("_XMLQCResult.xml"));
      // resultXMLFile = Result_xmlFile.toStdString();
      // }

      CIntensityMotionCheck IntensityMotionCheck;
      IntensityMotionCheck.SetXmlFileName(resultXMLFile);
      IntensityMotionCheck.SetDwiFileName(DWIFileName);
      QString         str( xmlFileName.c_str() );
      QFileInfo       xmlFile( str); // QString::fromStdString(xmlFileName) );
      XmlStreamReader XmlReader(NULL);
      XmlReader.setProtocol( &protocol);
      if( xmlFile.exists() )
        {
        XmlReader.readFile(str, XmlStreamReader::ProtocolWise);
        }

      protocol.GetQCOutputDirectory() = resultFolder;
      protocol.printProtocols();
      IntensityMotionCheck.SetProtocol( &protocol);
      IntensityMotionCheck.SetQCResult( &qcResult);
      IntensityMotionCheck.GetImagesInformation();
      if( !IntensityMotionCheck.GetDwiLoadStatus() )
        {
        std::cerr << "Failed to load DWI file " << DWIFileName << std::endl;
        std::cout << "FAILURE IN:" <<  __FILE__ << " at " <<  __LINE__ << std::endl;
        exit(1);
        }
      if( bCreateDefaultProtocol || !xmlFile.exists() )
        {
        IntensityMotionCheck.MakeDefaultProtocol( &protocol );
        protocol.GetQCOutputDirectory() = resultFolder;
        protocol.Save( xmlFileName.c_str() );
        }

      if( bcheckByProtocol || ( !bCreateDefaultProtocol && !bcheckByProtocol ) )
        {
        if( !bCreateDefaultProtocol && !bcheckByProtocol )
          {
          std::cout << "Create default protocol or check by protocol, at least one of them should be set." << std::endl;
          std::cout << "Neither of them was set. Check by default." << std::endl;
          }

        if (protocol.GetBrainMaskProtocol().BrainMask_Method == 2 && protocol.GetBrainMaskProtocol().BrainMask_Image.empty() && protocol.GetBrainMaskProtocol().bMask == true)
          {
    	  std::cerr << "The brain mask procedure needs brain mask image. No brain mask image is used in protocol." << std::endl;
          std::cout << "FAILURE IN:" <<  __FILE__ << " at " <<  __LINE__ << std::endl;
    	  exit(1);
          }

        const unsigned result = IntensityMotionCheck.RunPipelineByProtocol();

        QCResult *outResult = IntensityMotionCheck.GetQCResult();

        // .........................................................................................
        // ...............Building XML QCResult........................................
        // .........................................................................................

        QString Result_xmlFile(resultXMLFile.c_str() );

        QFile file(Result_xmlFile);

        if( !file.open(QFile::WriteOnly | QFile::Text) )
          {
          std::cerr << "Error: Cannot write file "
                    << qPrintable(Result_xmlFile) << ": "
                    << qPrintable( file.errorString() ) << std::endl;
          }

        QXmlStreamWriter xmlWriter(&file);
        xmlWriter.setAutoFormatting(true);
        xmlWriter.writeStartDocument();
        xmlWriter.writeStartElement("QCResultSettings");
        // ImageInformation
        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "ImageInformation" );
        if( protocol.GetImageProtocol().bCheck )
          {
          if( qcResult.GetImageCheckError() &&
              ( protocol.GetImageProtocol().bQuitOnCheckSizeFailure ||
                protocol.GetImageProtocol().bQuitOnCheckSpacingFailure ) )
            {
            xmlWriter.writeTextElement("value", "Fail Pipeline Terminated");
            xmlWriter.writeStartElement("entry");
            xmlWriter.writeAttribute( "parameter", "file name"  );
            xmlWriter.writeTextElement("value", qcResult.GetImageInformationCheckResult().info);
            xmlWriter.writeEndElement();
            xmlWriter.writeEndElement();
            xmlWriter.writeEndDocument();
            std::cout << "SUCCESSFUL execution of " << argv[0] << std::endl;
            return EXIT_SUCCESS;
            }
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "file name"  );
          xmlWriter.writeTextElement("value", qcResult.GetImageInformationCheckResult().info);
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "origin"  );
          if( qcResult.GetImageInformationCheckResult().origin )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "size"  );
          if( qcResult.GetImageInformationCheckResult().size )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "space"  );
          if( qcResult.GetImageInformationCheckResult().space )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "spacedirection"  );
          if( qcResult.GetImageInformationCheckResult().spacedirection )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "spacing"  );
          if( qcResult.GetImageInformationCheckResult().spacing )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeEndElement();
          }
        else if( !protocol.GetImageProtocol().bCheck )
          {
          xmlWriter.writeTextElement("value", "Info Not Check");
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "file name"  );
          xmlWriter.writeTextElement("value", qcResult.GetImageInformationCheckResult().info);
          xmlWriter.writeEndElement();
          xmlWriter.writeEndElement();
          }

        // DiffusionInformationCheckResult
        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "DiffusionInformation"  );

        if( protocol.GetDiffusionProtocol().bCheck )
          {

          // DiffusionInformationCheckResult
          if( protocol.GetDiffusionProtocol().bQuitOnCheckFailure && qcResult.GetDiffusionCheckError() )
            {
            xmlWriter.writeTextElement("value", "Fail Pipeline Terminated");
            std::cout << "SUCCESSFUL execution of " << argv[0] << std::endl;
            return EXIT_SUCCESS;;
            }

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "b value"  );
          if( qcResult.GetDiffusionInformationCheckResult().b )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "gradient"  );
          if( qcResult.GetDiffusionInformationCheckResult().gradient )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter", "measurementFrame"  );
          if( qcResult.GetDiffusionInformationCheckResult().measurementFrame )
            {
            xmlWriter.writeTextElement("value", "Pass");
            }
          else
            {
            xmlWriter.writeTextElement("value", "Fail");
            }
          xmlWriter.writeEndElement();
          }
        else if( !protocol.GetDiffusionProtocol().bCheck )
          {
          xmlWriter.writeTextElement("value", "Info Not Check");

          }
        xmlWriter.writeEndElement();
        // itemIntensityMotionInformation
        // Overall assesment
        // computing the overall results of the SliceWise check, InterlaceWise check and GradientWise check
        int num_SliceWiseCheckExc = 0;
        int num_InterlaceWiseCheckExc = 0;
        int num_GradientWiseCheckExc = 0;
        for( unsigned int i = 0; i < qcResult.GetIntensityMotionCheckResult().size();
             i++ )
          {

          if( qcResult.GetIntensityMotionCheckResult()[i].processing == QCResult::GRADIENT_EXCLUDE_SLICECHECK )
            {
            num_SliceWiseCheckExc++;
            }
          if( qcResult.GetIntensityMotionCheckResult()[i].processing == QCResult::GRADIENT_EXCLUDE_INTERLACECHECK )
            {
            num_InterlaceWiseCheckExc++;
            }
          if( qcResult.GetIntensityMotionCheckResult()[i].processing == QCResult::GRADIENT_EXCLUDE_GRADIENTCHECK )
            {
            num_GradientWiseCheckExc++;
            }
          }
        r_SliceWiseCkeck = num_SliceWiseCheckExc / qcResult.GetIntensityMotionCheckResult().size();
        r_InterlaceWiseCheck = num_InterlaceWiseCheckExc
          / (qcResult.GetIntensityMotionCheckResult().size() - num_SliceWiseCheckExc);
        r_GradWiseCheck = num_GradientWiseCheckExc
          / (qcResult.GetIntensityMotionCheckResult().size() - num_SliceWiseCheckExc - num_InterlaceWiseCheckExc);

        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "DWI Check"  );

        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "SliceWiseCheck"  );

        if( protocol.GetSliceCheckProtocol().bCheck )    // Check protocol whether run SliceWiseChecking
          {
          if( qcResult.GetSliceWiseCheckError() )
            {
            if( protocol.GetSliceCheckProtocol().bQuitOnCheckFailure )
              {
              xmlWriter.writeTextElement("value", "Fail Pipeline Termination");
              }
            else
              if( r_SliceWiseCkeck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
                {

                xmlWriter.writeTextElement("value", "Fail");
                }
              else
                {
                xmlWriter.writeTextElement("value", "Pass");
                }
            }
          else
            if( r_SliceWiseCkeck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
              {

              xmlWriter.writeTextElement("value", "Fail");

              }
            else
              {
              xmlWriter.writeTextElement("value", "Pass");
              }
          }
        else if( !protocol.GetSliceCheckProtocol().bCheck )
          {
          xmlWriter.writeTextElement("value", "Not Set");
          }

        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "InterlaceWiseCheck"  );

        if( !( qcResult.GetSliceWiseCheckError() && protocol.GetSliceCheckProtocol().bQuitOnCheckFailure ) &&
            protocol.GetInterlaceCheckProtocol().bCheck )
        // Check protocol whether run InterfaceWiseChecking
          {
          if(  qcResult.GetInterlaceWiseCheckError() )
            {
            if( protocol.GetInterlaceCheckProtocol().bQuitOnCheckFailure )
              {
              xmlWriter.writeTextElement("value", "Fail Pipeline Termination");
              }
            else
              {
              if( r_InterlaceWiseCheck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
                {
                xmlWriter.writeTextElement("value", "Fail");
                }
              else
                {
                xmlWriter.writeTextElement("value", "Pass");
                }
              }
            }
          else
            {
            if( r_InterlaceWiseCheck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
              {
              xmlWriter.writeTextElement("value", "Fail");
              }
            else
              {
              xmlWriter.writeTextElement("value", "Pass");
              }
            }
          }
        else if( !protocol.GetInterlaceCheckProtocol().bCheck )
          {
          xmlWriter.writeTextElement("value", "Not Set");
          }

        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter", "GradientWiseCheck"  );

        if( !protocol.GetSliceCheckProtocol().bQuitOnCheckFailure &&
            !protocol.GetInterlaceCheckProtocol().bQuitOnCheckFailure && protocol.GetGradientCheckProtocol().bCheck )                                                              //
                                                                                                                                                                                   //
                                                                                                                                                                                   // Check
                                                                                                                                                                                   //
                                                                                                                                                                                   // protocol
                                                                                                                                                                                   //
                                                                                                                                                                                   // whether
                                                                                                                                                                                   //
                                                                                                                                                                                   // run
                                                                                                                                                                                   //
                                                                                                                                                                                   // GradientWiseChecking
          {
          if( qcResult.GetGradientWiseCheckError() )
            {
            if( protocol.GetGradientCheckProtocol().bQuitOnCheckFailure )
              {

              xmlWriter.writeTextElement("value", "Fail Pipeline Terminatio");
              }
            else
              {
              if( r_GradWiseCheck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
                {
                xmlWriter.writeTextElement("value", "Fail");
                }
              else
                {
                xmlWriter.writeTextElement("value", "Pass");
                }
              }
            }
          else
            {
            if( r_GradWiseCheck > protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )
              {
              xmlWriter.writeTextElement("value", "Fail");
              }
            else
              {
              xmlWriter.writeTextElement("value", "Pass");
              }
            }

          }
        else if( !protocol.GetGradientCheckProtocol().bCheck )
          {
          xmlWriter.writeTextElement("value", "Not Set");
          }
        xmlWriter.writeEndElement();
        for( unsigned int i = 0;
             i < qcResult.GetIntensityMotionCheckResult().size();
             i++ )
          {

          // gradient
          bool EXCLUDE_SliceWiseCheck = false;
          bool EXCLUDE_InterlaceWiseCheck = false;
          bool EXCLUDE_GreadientWiseCheck = false;

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  QString("gradient_%1").arg( i, 4, 10, QLatin1Char( '0' ) ) );

          switch( qcResult.GetIntensityMotionCheckResult()[i].processing )
            {
            case QCResult::GRADIENT_BASELINE_AVERAGED:
              xmlWriter.writeTextElement("processing", "BASELINE_AVERAGED");
              break;
            case QCResult::GRADIENT_EXCLUDE_SLICECHECK:
            {
            xmlWriter.writeTextElement("processing", "EXCLUDE_SLICECHECK");
            EXCLUDE_SliceWiseCheck = true;
            }
            break;
            case QCResult::GRADIENT_EXCLUDE_INTERLACECHECK:
            {
            xmlWriter.writeTextElement("processing", "EXCLUDE_INTERLACECHECK");
            EXCLUDE_InterlaceWiseCheck = true;
            }
            break;
            case QCResult::GRADIENT_EXCLUDE_GRADIENTCHECK:
            {
            xmlWriter.writeTextElement("processing", "EXCLUDE_GRADIENTCHECK");
            EXCLUDE_GreadientWiseCheck = true;
            }
            break;
            case QCResult::GRADIENT_EXCLUDE_MANUALLY:
              xmlWriter.writeTextElement("processing", "EXCLUDE");
              break;
            case QCResult::GRADIENT_EDDY_MOTION_CORRECTED:
              xmlWriter.writeTextElement("processing", "EDDY_MOTION_CORRECTED");
              break;
            case QCResult::GRADIENT_INCLUDE:
            default:
              xmlWriter.writeTextElement("processing", "INCLUDE");
              break;
            }

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "OriginalDir" );
          xmlWriter.writeTextElement("value", QString("%1 %2 %3")
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].OriginalDir[0], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].OriginalDir[1], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].OriginalDir[2], 0, 'f',
                                          6) );
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "ReplacedDir" );
          xmlWriter.writeTextElement("value", QString("%1 %2 %3")
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].ReplacedDir[0], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].ReplacedDir[1], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].ReplacedDir[2], 0, 'f',
                                          6) );
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "CorrectedDir" );
          xmlWriter.writeTextElement("value", QString("%1 %2 %3")
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].CorrectedDir[0], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].CorrectedDir[1], 0, 'f',
                                          6)
                                     .arg(qcResult.GetIntensityMotionCheckResult()[i].CorrectedDir[2], 0, 'f',
                                          6) );
          xmlWriter.writeEndElement();
          std::vector< double > parameters = qcResult.GetIntensityMotionCheckResult()[i].EddyCurrentCorrectionTransform.Parameters ;
          if( !parameters.empty() )
          {
            xmlWriter.writeStartElement("entry");
            xmlWriter.writeAttribute( "parameter",  "TransformParameters" );
            QString transformToString = QString("%1").arg(parameters[0], 0, 'f', 8);
            for( size_t j = 1 ; j < parameters.size() ; j++ )
            {
              transformToString += " " + QString("%1").arg(parameters[j], 0, 'f', 8);
            }
            xmlWriter.writeTextElement("value", transformToString ) ;
            xmlWriter.writeEndElement();
          }
          std::vector< double > fixedParameters = qcResult.GetIntensityMotionCheckResult()[i].EddyCurrentCorrectionTransform.FixedParameters ;
          if( !fixedParameters.empty() )
          {
            xmlWriter.writeStartElement("entry");
            xmlWriter.writeAttribute( "parameter",  "TransformFixedParameters" );
            QString transformToString = QString("%1").arg(fixedParameters[0], 0, 'f', 8);
            for( size_t j = 1 ; j < fixedParameters.size() ; j++ )
            {
              transformToString += " " + QString("%1").arg(fixedParameters[j], 0, 'f', 8);
            }
            xmlWriter.writeTextElement("value", transformToString ) ;
            xmlWriter.writeEndElement();
          }
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "Translation" );
          double *translation = qcResult.GetIntensityMotionCheckResult()[i].EddyCurrentCorrectionTransform.Translation ;
          QString translationToString = QString("%1").arg(translation[0], 0, 'f', 8);
          for( int j = 1 ; j < 3 ; j++ )
          {
            translationToString += " " + QString("%1").arg(translation[j], 0, 'f', 8);
          }
          xmlWriter.writeTextElement("value", translationToString ) ;
          xmlWriter.writeEndElement();

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "TranslationNorm" );
          QString translationNormToString = QString("%1").arg(qcResult.GetIntensityMotionCheckResult()[i].EddyCurrentCorrectionTransform.TranslationNorm, 0, 'f', 8);
          xmlWriter.writeTextElement("value", translationNormToString ) ;
          xmlWriter.writeEndElement();

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "Angle" );
          QString AngleToString = QString("%1").arg(qcResult.GetIntensityMotionCheckResult()[i].EddyCurrentCorrectionTransform.Angle, 0, 'f', 8);
          xmlWriter.writeTextElement("value", AngleToString ) ;
          xmlWriter.writeEndElement();

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "SliceWiseCheck" );

          if( protocol.GetSliceCheckProtocol().bCheck )
            {
            if( EXCLUDE_SliceWiseCheck == true )
              {
              xmlWriter.writeTextElement("processing", "EXCLUDE");
              for( unsigned int S_index = 0; S_index < qcResult.GetSliceWiseCheckResult().size(); S_index++ )
                {
                if( i == static_cast<unsigned int>(qcResult.GetSliceWiseCheckResult()[S_index].GradientNum) )
                  {
                  xmlWriter.writeStartElement("entry");
                  xmlWriter.writeAttribute( "parameter",  "Slice#" );
                  xmlWriter.writeTextElement("value",
                                             QString("%1").arg(qcResult.GetSliceWiseCheckResult()[S_index].SliceNum) );
                  xmlWriter.writeEndElement();
                  xmlWriter.writeStartElement("entry");
                  xmlWriter.writeAttribute( "parameter",  "Correlation" );
                  xmlWriter.writeTextElement("value",
                                             QString("%1").arg(qcResult.GetSliceWiseCheckResult()[S_index].Correlation) );
                  xmlWriter.writeEndElement();
                  }
                }
              }
            if( EXCLUDE_SliceWiseCheck == false )
              {
              xmlWriter.writeTextElement("processing", "INCLUDE");
              }
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "InterlaceWiseCheck" );
          if( !qcResult.GetSliceWiseCheckError() || !protocol.GetSliceCheckProtocol().bQuitOnCheckFailure )
            {
            if( protocol.GetInterlaceCheckProtocol().bCheck )
              {
              if( EXCLUDE_InterlaceWiseCheck == true )
                {
                xmlWriter.writeTextElement("processing", "EXCLUDE");
                }
              else
                if( EXCLUDE_SliceWiseCheck == true )
                  {
                  xmlWriter.writeTextElement("processing", "NA");
                  }
                else
                  {
                  xmlWriter.writeTextElement("processing", "INCLUDE");
                  }

              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceAngleX" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].AngleX <
                  protocol.GetInterlaceCheckProtocol().rotationThreshold )
                {
                xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleX) ); //
                                                                                                                           //
                                                                                                                           // rotation
                                                                                                                           //
                                                                                                                           // threshold
                                                                                                                           //
                                                                                                                           // in
                                                                                                                           //
                                                                                                                           // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleX) ); //
                                                                                                                         //
                                                                                                                         // rotation
                                                                                                                         //
                                                                                                                         // threshold
                                                                                                                         //
                                                                                                                         // in
                                                                                                                         //
                                                                                                                         // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceAngleY" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].AngleY <
                  protocol.GetInterlaceCheckProtocol().rotationThreshold )
                {
                xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleY) ); //
                                                                                                                           //
                                                                                                                           // rotation
                                                                                                                           //
                                                                                                                           // threshold
                                                                                                                           //
                                                                                                                           // in
                                                                                                                           //
                                                                                                                           // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleY) ); //
                                                                                                                         //
                                                                                                                         // rotation
                                                                                                                         //
                                                                                                                         // threshold
                                                                                                                         //
                                                                                                                         // in
                                                                                                                         //
                                                                                                                         // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceAngleZ" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].AngleZ <
                  protocol.GetInterlaceCheckProtocol().rotationThreshold )
                {
                xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleZ) ); //
                                                                                                                           //
                                                                                                                           // rotation
                                                                                                                           //
                                                                                                                           // threshold
                                                                                                                           //
                                                                                                                           // in
                                                                                                                           //
                                                                                                                           // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].AngleZ) ); //
                                                                                                                         //
                                                                                                                         // rotation
                                                                                                                         //
                                                                                                                         // threshold
                                                                                                                         //
                                                                                                                         // in
                                                                                                                         //
                                                                                                                         // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceTranslationX" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].TranslationX <
                  protocol.GetInterlaceCheckProtocol().translationThreshold )
                {
                xmlWriter.writeTextElement("green",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationX) );        //
                                                                                                                               //
                                                                                                                               // translation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationX) );      //
                                                                                                                             //
                                                                                                                             // translation
                                                                                                                             //
                                                                                                                             // threshold
                                                                                                                             //
                                                                                                                             // in
                                                                                                                             //
                                                                                                                             // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceTranslationY" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].TranslationY <
                  protocol.GetInterlaceCheckProtocol().translationThreshold )
                {
                xmlWriter.writeTextElement("green",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationY) );        //
                                                                                                                               //
                                                                                                                               // translation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationY) );      //
                                                                                                                             //
                                                                                                                             // translation
                                                                                                                             //
                                                                                                                             // threshold
                                                                                                                             //
                                                                                                                             // in
                                                                                                                             //
                                                                                                                             // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceTranslationZ" );
              if( qcResult.GetInterlaceWiseCheckResult()[i].TranslationZ <
                  protocol.GetInterlaceCheckProtocol().translationThreshold )
                {
                xmlWriter.writeTextElement("green",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationZ) );        //
                                                                                                                               //
                                                                                                                               // translation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                }
              else
                {
                xmlWriter.writeTextElement("red",
                                           QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].TranslationZ) );      //
                                                                                                                             //
                                                                                                                             // translation
                                                                                                                             //
                                                                                                                             // threshold
                                                                                                                             //
                                                                                                                             // in
                                                                                                                             //
                                                                                                                             // protocol
                }
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "InterlaceMetric(MI)" );
              xmlWriter.writeTextElement("value", QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].Metric) );
              xmlWriter.writeEndElement();
              xmlWriter.writeStartElement("entry");
              if( i == 0 ) // baseline
                {
                xmlWriter.writeAttribute( "parameter",  "InterlaceCorrelation_Baseline" );
                if( qcResult.GetInterlaceWiseCheckResult()[i].Correlation <
                    protocol.GetInterlaceCheckProtocol().correlationThresholdBaseline )                                                          //
                                                                                                                                                 //
                                                                                                                                                 // correlation
                                                                                                                                                 //
                                                                                                                                                 // threshold
                                                                                                                                                 //
                                                                                                                                                 // for
                                                                                                                                                 //
                                                                                                                                                 // baseline
                                                                                                                                                 //
                                                                                                                                                 // in
                                                                                                                                                 //
                                                                                                                                                 // protocol
                  {
                  xmlWriter.writeTextElement("green",
                                             QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].Correlation) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red",
                                             QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].Correlation) );
                  }

                }
              else
                {
                xmlWriter.writeAttribute( "parameter",  "InterlaceCorrelation" );
                if( qcResult.GetInterlaceWiseCheckResult()[i].Correlation <
                    protocol.GetInterlaceCheckProtocol().correlationThresholdGradient )                                                          //
                                                                                                                                                 //
                                                                                                                                                 // correlation
                                                                                                                                                 //
                                                                                                                                                 // threshold
                                                                                                                                                 //
                                                                                                                                                 // gradients
                                                                                                                                                 //
                                                                                                                                                 // in
                                                                                                                                                 //
                                                                                                                                                 // protocol
                  {
                  xmlWriter.writeTextElement("green",
                                             QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].Correlation) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red",
                                             QString("%1").arg(qcResult.GetInterlaceWiseCheckResult()[i].Correlation) );
                  }
                }
              xmlWriter.writeEndElement();
              }
            xmlWriter.writeEndElement();
            xmlWriter.writeStartElement("entry");
            xmlWriter.writeAttribute( "parameter",  "GradientWiseCheck" );
            if( !qcResult.GetInterlaceWiseCheckError() ||
                !( protocol.GetSliceCheckProtocol().bQuitOnCheckFailure ||
                   protocol.GetInterlaceCheckProtocol().bQuitOnCheckFailure ) )
              {

              if( protocol.GetGradientCheckProtocol().bCheck )
                {
                if( EXCLUDE_GreadientWiseCheck == true )
                  {
                  xmlWriter.writeTextElement("value", "EXCLUDE");
                  }
                else if( EXCLUDE_SliceWiseCheck == true )
                  {
                  xmlWriter.writeTextElement("value", "NA");
                  }
                else
                  {
                  xmlWriter.writeTextElement("value", "INCLUDE");
                  }
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientAngleX" );
                if( qcResult.GetGradientWiseCheckResult()[i].AngleX <
                    protocol.GetGradientCheckProtocol().rotationThreshold )                                                    //
                                                                                                                               //
                                                                                                                               // rotation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                  {
                  xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleX) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleX) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientAngleY" );
                if( qcResult.GetGradientWiseCheckResult()[i].AngleY <
                    protocol.GetGradientCheckProtocol().rotationThreshold )                                                    //
                                                                                                                               //
                                                                                                                               // rotation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                  {
                  xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleY) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleY) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientAngleZ" );
                if( qcResult.GetGradientWiseCheckResult()[i].AngleZ <
                    protocol.GetGradientCheckProtocol().rotationThreshold )                                                    //
                                                                                                                               //
                                                                                                                               // rotation
                                                                                                                               //
                                                                                                                               // threshold
                                                                                                                               //
                                                                                                                               // in
                                                                                                                               //
                                                                                                                               // protocol
                  {
                  xmlWriter.writeTextElement("green", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleZ) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red", QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].AngleZ) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientTranslationX" );
                if( qcResult.GetGradientWiseCheckResult()[i].TranslationX <
                    protocol.GetGradientCheckProtocol().translationThreshold )                                                           //
                                                                                                                                         //
                                                                                                                                         // translation
                                                                                                                                         //
                                                                                                                                         // threshold
                                                                                                                                         //
                                                                                                                                         // in
                                                                                                                                         //
                                                                                                                                         // protocol
                  {
                  xmlWriter.writeTextElement("green",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationX) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationX) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientTranslationY" );
                if( qcResult.GetGradientWiseCheckResult()[i].TranslationY <
                    protocol.GetGradientCheckProtocol().translationThreshold )                                                           //
                                                                                                                                         //
                                                                                                                                         // translation
                                                                                                                                         //
                                                                                                                                         // threshold
                                                                                                                                         //
                                                                                                                                         // in
                                                                                                                                         //
                                                                                                                                         // protocol
                  {
                  xmlWriter.writeTextElement("green",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationY) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationY) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientTranslationZ" );
                if( qcResult.GetGradientWiseCheckResult()[i].TranslationZ <
                    protocol.GetGradientCheckProtocol().translationThreshold )                                                           //
                                                                                                                                         //
                                                                                                                                         // translation
                                                                                                                                         //
                                                                                                                                         // threshold
                                                                                                                                         //
                                                                                                                                         // in
                                                                                                                                         //
                                                                                                                                         // protocol
                  {
                  xmlWriter.writeTextElement("green",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationZ) );
                  }
                else
                  {
                  xmlWriter.writeTextElement("red",
                                             QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].TranslationZ) );
                  }
                xmlWriter.writeEndElement();
                xmlWriter.writeStartElement("entry");
                xmlWriter.writeAttribute( "parameter",  "GradientMetric(MI)" );
                xmlWriter.writeTextElement("value",
                                           QString("%1").arg(qcResult.GetGradientWiseCheckResult()[i].MutualInformation) );
                xmlWriter.writeEndElement();
                }
              }
            else
              {
              xmlWriter.writeTextElement("processing", "NA");
              }
            xmlWriter.writeEndElement();

            }
          else
            {
            xmlWriter.writeTextElement("processing", "NA");
            xmlWriter.writeEndElement();
            xmlWriter.writeStartElement("entry");
            xmlWriter.writeAttribute( "parameter",  "GradientWiseCheck" );
            xmlWriter.writeTextElement("processing", "NA");
            xmlWriter.writeEndElement();
            }

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "QC_Index" );
          xmlWriter.writeTextElement("value", QString("%1").arg(qcResult.GetIntensityMotionCheckResult()[i].QCIndex) );
          xmlWriter.writeEndElement();

          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  "Visual Check" );
          xmlWriter.writeStartElement("entry");
          xmlWriter.writeAttribute( "parameter",  QString("VC_Status_%1").arg( i, 4, 10, QLatin1Char( '0' ) ) );

          switch( qcResult.GetIntensityMotionCheckResult()[i].VisualChecking )
            {
            // case QCResult::GRADIENT_BASELINE_AVERAGED:
            // xmlWriter.writeTextElement("value","BASELINE_AVERAGED");
            // break;
            // case QCResult::GRADIENT_EXCLUDE_SLICECHECK:
            // {
            // xmlWriter.writeTextElement("value","EXCLUDE_SLICECHECK");
            // EXCLUDE_SliceWiseCheck=true;
            // }
            // break;
            // case QCResult::GRADIENT_EXCLUDE_INTERLACECHECK:
            // {
            // xmlWriter.writeTextElement("value","EXCLUDE_INTERLACECHECK");
            // EXCLUDE_InterlaceWiseCheck=true;
            // }
            // break;
            // case QCResult::GRADIENT_EXCLUDE_GRADIENTCHECK:
            // {
            // xmlWriter.writeTextElement("value","EXCLUDE_GRADIENTCHECK");
            // EXCLUDE_GreadientWiseCheck=true;
            // }
            // break;
            case QCResult::GRADIENT_EXCLUDE_MANUALLY:
              xmlWriter.writeTextElement("value", "EXCLUDE");
              break;
              // case QCResult::GRADIENT_EDDY_MOTION_CORRECTED:
              // xmlWriter.writeTextElement("value","EDDY_MOTION_CORRECTED");
              // break;
            case QCResult::GRADIENT_INCLUDE:
              xmlWriter.writeTextElement("value", "INCLUDE");
              break;
            default:
              xmlWriter.writeTextElement("value", "NoChange");
              break;
            }
          xmlWriter.writeEndElement();
          xmlWriter.writeEndElement();

          xmlWriter.writeEndElement();
          }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter",  "BRAIN_MASK" );


        //xmlWriter.writeTextElement("value", "Not Set");

        if( protocol.GetBrainMaskProtocol().bQuitOnCheckFailure && qcResult.GetBrainMaskCheckError() )
          {
          xmlWriter.writeTextElement("value", "Fail Pipeline Terminated");
          }
        else
          {
          if( !protocol.GetBrainMaskProtocol().bMask )
            {
            xmlWriter.writeTextElement("value", "Not Set");
            }
          else
            {
            if( qcResult.GetOverallQCResult().BMCK == true )
              {
              xmlWriter.writeTextElement("value", "Pass");
              }
            else
              {
              xmlWriter.writeTextElement("value", "Fail");
              }

            }
          }
        xmlWriter.writeEndElement();


        xmlWriter.writeStartElement("entry");
        xmlWriter.writeAttribute( "parameter",  "Dominant_Direction_Detector" );
        if(  protocol.GetDominantDirectional_Detector().bQuitOnCheckFailure &&
             qcResult.GetDominantDirectionalCheckError() )
          {
          xmlWriter.writeTextElement("value", "Fail Pipeline Terminated");
          }

        else
          {
          if( !protocol.GetDominantDirectional_Detector().bCheck )
            {
            xmlWriter.writeTextElement("value", "Not Set");
            }
          else
            {
            if( qcResult.GetOverallQCResult().DDDCK == true )
              {
              xmlWriter.writeTextElement("value", "Pass");


              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "DOMINANT_DIRECTION_Z_SCORE" );
              xmlWriter.writeTextElement("value", QString("%1").arg(qcResult.GetDominantDirection_Detector().z_score) );
              xmlWriter.writeEndElement();

              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "DOMINANT_DIRECTION_ENTROPY" );
              xmlWriter.writeTextElement("value", QString("%1").arg(qcResult.GetDominantDirection_Detector().entropy_value)  );
              xmlWriter.writeEndElement();

              xmlWriter.writeStartElement("entry");
              xmlWriter.writeAttribute( "parameter",  "DOMINANT_DIRECTION_RESULT" );
              if( qcResult.GetDominantDirection_Detector().detection_result == 2 )
                {
                xmlWriter.writeTextElement("value",  "Reject" );
                }

              if( qcResult.GetDominantDirection_Detector().detection_result == 1 )
                {
                xmlWriter.writeTextElement("value", "Suspicious" );
                }

              if( qcResult.GetDominantDirection_Detector().detection_result == 0 )
                {
                xmlWriter.writeTextElement("value",  "Accept" );
                }
              }
            else
              {
              xmlWriter.writeTextElement("value", "Fail");
              }
            xmlWriter.writeEndElement();

            }


          }

        xmlWriter.writeEndElement();

        xmlWriter.writeEndDocument();
        file.close();
        if( file.error() )
          {
          std::cerr << "Error: Cannot write file "
                    << qPrintable(Result_xmlFile) << ": "
                    << qPrintable( file.errorString() ) << std::endl;
          }

        if( resultNotes.length() > 0 )
          {
          std::ofstream outfile;

          QFileInfo noteFile( QString::fromStdString(resultNotes) );
          if( !noteFile.exists() )
            {
            outfile.open( resultNotes.c_str(),  std::ios::app);
            outfile
              <<
              "DWI\t#AllGrad/#AllGradLeft\t#b0/#b0Left\t#Grad/#GradLeft\t#GradDir/#GradDirLeft\t#Rep/#RepLeft\tImageInfo\tDiffusionInfo\tSliceWise\tInterlaceWise\tGradWise\tGradDirLess6\tSingleBValueNoB0\tTooManyBadDirs";
            }
          else
            {
            outfile.open( resultNotes.c_str(),  std::ios::app);
            }

          if( DWIFileName.rfind('/') != string::npos )
            {
            std::cout << DWIFileName.substr( DWIFileName.rfind(
                                               '/') + 1, DWIFileName.length() - DWIFileName.rfind('/')
                                             - 1) << std::endl;
            outfile << std::endl << DWIFileName.substr( DWIFileName.rfind(
                                                          '/') + 1, DWIFileName.length() - DWIFileName.rfind('/') - 1);
            }
          else
            {
            std::cout << "DWI file name: " << DWIFileName << std::endl;
            outfile << std::endl << DWIFileName;
            }

          std::cout << "GradientTotal#/LeftGradientTotal#: "
                    << IntensityMotionCheck.getGradientNumber()
            + IntensityMotionCheck.getBaselineNumber() << "/"
                    << IntensityMotionCheck.
            getGradientLeftNumber()
            + IntensityMotionCheck.getBaselineLeftNumber() << std::endl;
          outfile << "\t" << IntensityMotionCheck.getGradientNumber()
            + IntensityMotionCheck.getBaselineNumber() << "/"
                  << IntensityMotionCheck.
            getGradientLeftNumber()
            + IntensityMotionCheck.getBaselineLeftNumber();

          std::cout << "Baseline#/LeftBaseline#: "
                    << IntensityMotionCheck.getBaselineNumber() << "/"
                    <<  IntensityMotionCheck.getBaselineLeftNumber()
                    << std::endl;
          outfile << "\t" << IntensityMotionCheck.getBaselineNumber() << "/"
                  <<  IntensityMotionCheck.getBaselineLeftNumber();

          std::cout << "Gradient#/LeftGradient#: "
                    << IntensityMotionCheck.getGradientNumber() << "/"
                    << IntensityMotionCheck.getGradientLeftNumber() << std::endl;
          outfile << "\t" << IntensityMotionCheck.getGradientNumber() << "/"
                  << IntensityMotionCheck.getGradientLeftNumber();

          std::cout << "GradientDir#/LeftGradientDir#: "
                    << IntensityMotionCheck.getGradientDirNumber() << "/"
                    << IntensityMotionCheck.getGradientDirLeftNumber()
                    << std::endl;
          outfile << "\t" << IntensityMotionCheck.getGradientDirNumber()
                  << "/" << IntensityMotionCheck.getGradientDirLeftNumber();

          std::cout << "Repetition#/RepetitionLeft#: "
                    << IntensityMotionCheck.getRepetitionNumber() << "/-"
                    << std::endl;
          outfile << "\t" << IntensityMotionCheck.getRepetitionNumber() << "/-";

          if( outResult->GetImageCheckError())
            {
            std::cout << "Image information check: FAILURE" << std::endl;
            outfile << "Image information check: FAILURE" << std::endl;
            }
          else
            {
            std::cout << "Image information check: PASS" << std::endl;
            outfile << "Image information check: PASS" << std::endl;
            }

          if( outResult->GetDiffusionCheckError() )
            {
            std::cout << "Diffusion information check: FAILURE" << std::endl;
            outfile << "Diffusion information check: FAILURE" << std::endl;
            }
          else
            {
            std::cout << "Diffusion information check: PASS" << std::endl;
            outfile << "Diffusion information check: PASS" << std::endl;
            }

          if( outResult->GetSliceWiseCheckError()  )
            {
            std::cout << "Slice-wise check: FAILURE" << std::endl;
            outfile << "Slice-wise check: FAILURE" << std::endl;
            }
          else
            {
            std::cout << "Slice-wise check: PASS" << std::endl;
            outfile <<  "Slice-wise check: PASS" << std::endl;
            }

          if( outResult->GetInterlaceWiseCheckError()  )
            {
            std::cout << "Interlace-wise check: FAILURE" <<  std::endl;
            outfile << "Interlace-wise check: FAILURE" <<  std::endl;
            }
          else
            {
            std::cout << "Interlace-wise check: PASS" << std::endl;
            outfile << "Interlace-wise check: PASS" << std::endl;
            }

          if( outResult->GetGradientWiseCheckError() )
            {
            std::cout << "Gradient-wise check: FAILURE" << std::endl;
            outfile << "Gradient-wise check: FAILURE" << std::endl;
            }
          else
            {
            std::cout << "Gradient-wise check: PASS" << std::endl;
            outfile << "\tGradient-wise check: PASS";
            }

          if( outResult->GetBrainMaskCheckError() )
            {
            std::cout << "Brain mask check:\t\tFAILURE" << std::endl;
            outfile << "Brain mask check:\t\tFAILURE";
            }
          else
            {
            std::cout << "Brain mask check: PASS" << std::endl;
            outfile << "Brain mask check: PASS" << std::endl;
            }

          if( outResult->GetDominantDirectionalCheckError() )
            {
            std::cout << "Dominant Directional Detector:\t\tFAILURE" << std::endl;
            outfile << "Dominant Directional Detector:\t\tFAILURE";
            }
          else
            {
            std::cout << "Dominant Directional Detector: PASS" << std::endl;
            outfile << "Dominant Directional Detector: PASS" << std::endl;
            }

          // ZYXEDCBA:
          // X QC;Too many bad gradient directions found!
          // Y QC; Single b-value DWI without a b0/baseline!
          // Z QC: Gradient direction #is less than 6!
          if( outResult->GetBadGradientCheckError() )
            {
            std::cout << "Too many bad gradient directions found!:  FAILURE" << std::endl;
            outfile << "\tToo many bad gradient directions found!:  FAILURE";
            }
          else
            {
            outfile << "\t";
            }

          if( outResult->GetBaselineLeftCheckError() )
            {
            std::cout << "Single b-value DWI without a b0/baseline!: FAILURE" << std::endl;
            outfile << "\tSingle b-value DWI without a b0/baseline!: FAILURE";
            }
          else
            {
            outfile << "\t";
            }

          if( outResult->GetGradientLeftCheckError() )
            {
            std::cout << "Gradient direction #is less than 6!: FAILURE" << std::endl;
            outfile << "\tGradient direction #is less than 6!: FAILURE";
            }
          else
            {
            outfile << "\t";
            }
          outfile.close();
          }
        if ( result == EXIT_SUCCESS )
          {
          std::cout << "SUCCESSFUL execution of " << argv[0] << std::endl;
          }
        else
          {
          std::cout << "FAILED execution of " << argv[0] << " in " << __FILE__ << " at " << __LINE__ << " with code " << static_cast<int>(result) << std::endl;
          }
         //HACK: Always return success here.
         return EXIT_SUCCESS;
        }
      }
    else
      {
      std::cout << "No protocol is loaded." << std::endl;
      }
    std::cout << "SUCCESSFUL execution of " << argv[0] << std::endl;
    return EXIT_SUCCESS;
    }
}
