#pragma once

#include "dcmtk/config/osconfig.h"

#ifdef HAVE_WINDOWS_H
#include <direct.h>        /* for _mkdir() */
#endif

#include "dcmtk/dcmnet/dimse.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofdatime.h"
#include "dcmtk/dcmnet/dicom.h"
#include "bridge.h"

static bool connected = FALSE;
DcmTagKey keyTransferSyntaxUid(0x2, 0x10);
DcmTagKey keySopClaUid(0x8, 0x16), keySopInsUid(0x8, 0x18), keyStuDat(0x8, 0x20), keySerDat(0x8, 0x21), keyStuTim(0x8, 0x30), 
          keySerTim(0x8, 0x31), keyAccNum(0x8, 0x50), keySeriesModality(0x8, 0x60), keyStudyModality(0x8, 0x61), 
          keyManufacturer(0x8, 0x70), keyInstNam(0x8, 0x80), keyRefPhyNam(0x8, 0x90), keyStationName(0x8, 0x1010), 
          keyStuDes(0x8, 0x1030), keySerDes(0x8, 0x103e), keyInstDepartName(0x8, 0x1040), keyPhyRecNam(0x8,0x1048),
          keyPerformPhyName(0x8, 0x1050), keyPhyReadStuName(0x8, 0x1060), keyOperateName(0x8, 0x1070), keyManufactModNam(0x8, 0x1090);
DcmTagKey keyPatNam(0x10, 0x10)/*keyPatNamKan, keyPatNamKat*/, keyPatId(0x10, 0x20), keyPatBirDat(0x10, 0x30), keyPatSex(0x10, 0x40),
          keyPatAge(0x10, 0x1010), keyPatSiz(0x10, 0x1020), keyPatWei(0x10, 0x1030);
DcmTagKey keyContBolus(0x8, 0x10), keyBodParExam(0x18, 0x15), keyProtocolNam(0x18, 0x1030), keyPatPos(0x18, 0x5100), keyViewPos(0x18, 0x5101);
DcmTagKey keyStuInsUid(0x20, 0xd), keySerInsUid(0x20, 0xe), keyStuId(0x20, 0x10), keySerNum(0x20, 0x11), keyImaNum(0x20, 0x13);
DcmTagKey keyReqPhysician(0x32, 0x1032), keyReqService(0x32, 0x1933);

void getValue(DcmDataset *imageDataSet, const char **pValue, DcmTagKey& key, OFString& value)
{
  imageDataSet->findAndGetOFString(key, value);
  *pValue = value.c_str();
  printf("%s : %s\n", key.toString().c_str(), *pValue);
}

void getNameValue(DcmDataset *imageDataSet, PImgDataset pDataset, DcmTagKey& key, OFString& value, OFString& valueKan, OFString& valueKat)
{
  OFString temp; 
  imageDataSet->findAndGetOFString(key, temp);
  bool isKan = false;
  for(int i; i < temp.length(); i++)
  {
    if(temp[i] > '~')
    {
      isKan = true;
      break;
    }
  }
  if(isKan)
    valueKan = temp;
  else
    value = temp;
  pDataset->pPatNam = value.c_str();
  pDataset->pPatNamKan = valueKan.c_str();
  pDataset->pPatNamKat = valueKat.c_str();
  printf("%s : %s\n", key.toString().c_str(), temp.c_str());
}

void getDateNumberValue(DcmDataset *imageDataSet, int &intValue, DcmTagKey& key, OFString& value)
{
  imageDataSet->findAndGetOFString(key, value);
  if(value.empty())
  {
    intValue = 0;
  }
  else
  {
    intValue = atoi(value.c_str());
  }
  printf("%s : DA %d\n", key.toString().c_str(), intValue);
}

void getTimeNumberValue(DcmDataset *imageDataSet, int &intValue, DcmTagKey& key, OFString& value)
{
  imageDataSet->findAndGetOFString(key, value);
  if(value.empty())
  {
    intValue = 0;
  }
  else
  {
    if(value.length() < 6) value.append(6 - value.length(), '0'); // see DICOM part5: TM data type
    int temp = atof(value.c_str());
    intValue = temp / 10000 * 3600 + temp % 10000 /100 * 60 + temp % 100;
  }
  printf("%s : TM %d\n", key.toString().c_str(), intValue);
}

void commitToDB()
{
  commitDicomDB();
}

bool insertImage(DcmDataset *imageDataSet, OFString& imageManageNumber, OFString& outputDirectory, OFString& relateFilePathName)
{
  char buff[1024];
  ImgDataset dataset;
  ::ZeroMemory(&dataset, sizeof(ImgDataset));

  OFString filePath = outputDirectory;
  filePath += PATH_SEPARATOR;
  filePath.append(relateFilePathName);

  ::GetCurrentDirectory(1024, buff);
  OFString hddRoot = buff;
  hddRoot += PATH_SEPARATOR;
  hddRoot += outputDirectory;

  dataset.pHddRoot = hddRoot.c_str();
  dataset.pPath = relateFilePathName.c_str();
  dataset.pImgManageNum = imageManageNumber.c_str();

  FILETIME fileTime;
  SYSTEMTIME utcTime, localTime;

  OFDateTime dateTime;
  dateTime.setCurrentDateTime();
  dataset.insertDate = dateTime.getDate().getYear() * 10000 + dateTime.getDate().getMonth() * 100 + dateTime.getDate().getDay();
  dataset.insertTime = dateTime.getTime().getTimeInSeconds();

  dataset.fileDate = 0;
  dataset.fileTime = 0;
  HANDLE handle = ::CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if( handle != INVALID_HANDLE_VALUE) 
  {
    if(::GetFileTime(handle, NULL, NULL, &fileTime))
    {
      FileTimeToSystemTime(&fileTime, &utcTime);
      SystemTimeToTzSpecificLocalTime(NULL, &utcTime, &localTime);
      dataset.fileDate = localTime.wYear * 10000 + localTime.wMonth * 100 + localTime.wDay;
      dataset.fileTime = localTime.wHour * 3600 + localTime.wMinute * 60 + localTime.wSecond;
    }
    ::GetFileSizeEx(handle, &dataset.fileSize);
    ::CloseHandle(handle);
  }

  if(dataset.fileDate == 0)
  { // open file failed, substitute at insertDate/Time
    dataset.fileDate = dataset.insertDate;
    dataset.fileTime = dataset.insertTime;
  }

  OFString valueTransferSyntaxUid;
  OFString valueSopClaUid;
  OFString valueSopInsUid;
  OFString valueStuDat; // int
  OFString valueSerDat; // int
  OFString valueStuTim; // int
  OFString valueSerTim; // int
  OFString valueAccNum;
  OFString valueSeriesModality;
  OFString valueStudyModality;
  OFString valueManufacturer;
  OFString valueInstNam;
  OFString valueRefPhyNam;
  OFString valueStationName;
  OFString valueStuDes;
  OFString valueSerDes;
  OFString valueInstDepartName;
  OFString valuePhyRecNam;
  OFString valuePerformPhyName;
  OFString valuePhyReadStuName;
  OFString valueOperateName;
  OFString valueManufactModNam;
  OFString valuePatNam;
  OFString valuePatNamKan;
  OFString valuePatNamKat;
  OFString valuePatId;
  OFString valuePatBirDat;
  OFString valuePatSex;
  OFString valuePatAge;
  OFString valuePatSiz;
  OFString valuePatWei;
  OFString valueContBolus;
  OFString valueBodParExam;
  OFString valueProtocolNam;
  OFString valuePatPos;
  OFString valueViewPos;
  OFString valueStuInsUid;
  OFString valueSerInsUid;
  OFString valueStuId;
  OFString valueSerNum;
  OFString valueImaNum;
  OFString valueReqPhysician;
  OFString valueReqService;

  getValue(imageDataSet, &(dataset.pTransferSyntaxUid), keyTransferSyntaxUid, valueTransferSyntaxUid);
  getValue(imageDataSet, &(dataset.pSopClaUid), keySopClaUid, valueSopClaUid);
  getValue(imageDataSet, &(dataset.pSopInsUid), keySopInsUid, valueSopInsUid);
  getDateNumberValue(imageDataSet, dataset.stuDat, keyStuDat, valueStuDat);
  getDateNumberValue(imageDataSet, dataset.serDat, keySerDat, valueSerDat);
  getTimeNumberValue(imageDataSet, dataset.stuTim, keyStuTim, valueStuTim);
  getTimeNumberValue(imageDataSet, dataset.serTim, keySerTim, valueSerTim);
  getValue(imageDataSet, &(dataset.pAccNum), keyAccNum, valueAccNum);
  getValue(imageDataSet, &(dataset.pSeriesModality), keySeriesModality, valueSeriesModality);
  getValue(imageDataSet, &(dataset.pStudyModality), keyStudyModality, valueStudyModality);
  getValue(imageDataSet, &(dataset.pManufacturer), keyManufacturer, valueManufacturer);
  getValue(imageDataSet, &(dataset.pInstNam), keyInstNam, valueInstNam);
  getValue(imageDataSet, &(dataset.pRefPhyNam), keyRefPhyNam, valueRefPhyNam);
  getValue(imageDataSet, &(dataset.pStationName), keyStationName, valueStationName);
  getValue(imageDataSet, &(dataset.pStuDes), keyStuDes, valueStuDes);
  getValue(imageDataSet, &(dataset.pSerDes), keySerDes, valueSerDes);
  getValue(imageDataSet, &(dataset.pInstDepartName), keyInstDepartName, valueInstDepartName);
  getValue(imageDataSet, &(dataset.pPhyRecNam), keyPhyRecNam, valuePhyRecNam);
  getValue(imageDataSet, &(dataset.pPerformPhyName), keyPerformPhyName, valuePerformPhyName);
  getValue(imageDataSet, &(dataset.pPhyReadStuName), keyPhyReadStuName, valuePhyReadStuName);
  getValue(imageDataSet, &(dataset.pOperateName), keyOperateName, valueOperateName);
  getValue(imageDataSet, &(dataset.pManufactModNam), keyManufactModNam, valueManufactModNam);
  getNameValue(imageDataSet, &dataset, keyPatNam, valuePatNam, valuePatNamKan, valuePatNamKat);
  getValue(imageDataSet, &(dataset.pPatId), keyPatId, valuePatId);
  getDateNumberValue(imageDataSet, dataset.patBirDat, keyPatBirDat, valuePatBirDat);
  getValue(imageDataSet, &(dataset.pPatSex), keyPatSex, valuePatSex);
  getValue(imageDataSet, &(dataset.pPatAge), keyPatAge, valuePatAge);
  getValue(imageDataSet, &(dataset.pPatSiz), keyPatSiz, valuePatSiz);
  getValue(imageDataSet, &(dataset.pPatWei), keyPatWei, valuePatWei);
  getValue(imageDataSet, &(dataset.pContBolus), keyContBolus, valueContBolus);
  getValue(imageDataSet, &(dataset.pBodParExam), keyBodParExam, valueBodParExam);
  getValue(imageDataSet, &(dataset.pProtocolNam), keyProtocolNam, valueProtocolNam);
  getValue(imageDataSet, &(dataset.pPatPos), keyPatPos, valuePatPos);
  getValue(imageDataSet, &(dataset.pViewPos), keyViewPos, valueViewPos);
  getValue(imageDataSet, &(dataset.pStuInsUid), keyStuInsUid, valueStuInsUid);
  getValue(imageDataSet, &(dataset.pSerInsUid), keySerInsUid, valueSerInsUid);
  getValue(imageDataSet, &(dataset.pStuId), keyStuId, valueStuId);
  getValue(imageDataSet, &(dataset.pSerNum), keySerNum, valueSerNum);
  getValue(imageDataSet, &(dataset.pImaNum), keyImaNum, valueImaNum);
  getValue(imageDataSet, &(dataset.pReqPhysician), keyReqPhysician, valueReqPhysician);
  getValue(imageDataSet, &(dataset.pReqService), keyReqService, valueReqService);

  if(valueSeriesModality.empty() && !valueStudyModality.empty())
  {
    valueSeriesModality = valueStudyModality;
    dataset.pSeriesModality = valueSeriesModality.c_str();
    printf("%s : %s\n", keySeriesModality.toString().c_str(), dataset.pSeriesModality);
  }
  else if(!valueSeriesModality.empty() && valueStudyModality.empty())
  {
    valueStudyModality = valueSeriesModality;
    dataset.pStudyModality = valueStudyModality.c_str();
    printf("%s : %s\n", keyStudyModality.toString().c_str(), dataset.pStudyModality);
  }

  if(connected ? connected : connectDicomDB())
  {
    return insertImageInfoToDB(&dataset);
  }
  return false;
}

void generateImageStoreDirectory(DcmDataset **imageDataSet, OFString& subdirectoryName, OFString& imageManageNumber)
{
  char buf[256];
  OFString studyInstUID, studyDate;
  //size_t pixelCounter = 0;
  //if(imageDataSet) (*imageDataSet)->print(COUT, 1, 0, NULL, &pixelCounter);

  OFDateTime currentDate;
  currentDate.setCurrentDateTime();

  if(imageDataSet)
  {
    (*imageDataSet)->findAndGetOFString(keyStuInsUid, studyInstUID);
    (*imageDataSet)->findAndGetOFString(keyStuDat, studyDate);
  }

  if(connected ? connected : connectDicomDB())
  {
    if(studyDate.empty())
    { // current date as default
      sprintf(buf, "%04u%02u%02u", currentDate.getDate().getYear(), currentDate.getDate().getMonth(), currentDate.getDate().getDay());
      studyDate = buf;  // YYYYMMDD
    }

    strncpy(buf, studyDate.c_str() + 2, 4); // YYMM
    buf[4] = PATH_SEPARATOR; // YYMM/
    if( ! getManageNumber(buf + 5, // old study's imageManageNumber or YYMM/YYYYMMDDXXXXXX
      studyInstUID.empty() ? NULL : studyInstUID.c_str(), atoi(studyDate.c_str())))
      goto failback;
  }
  else // no db seq value, using current time format: NODB/YYYMMDD_XXXXX, XXXXX = seconds in today
  {
failback:
    strncpy(buf, "NODB", 4); // NODB
    buf[4] = PATH_SEPARATOR; // NODB/
    sprintf(buf + 5, "%04u%02u%02u_%05u", // NODB/YYYMMDD_XXXXX
      currentDate.getDate().getYear(), currentDate.getDate().getMonth(), currentDate.getDate().getDay(),
      (int)currentDate.getTime().getTimeInSeconds()); 
  }

  // subdirectoryName = "20"
  subdirectoryName += buf;
  imageManageNumber = &buf[5];
}

DIC_US mkdirRecursive(OFString& subdirectoryPath)
{
  // check if the subdirectory is already existent
  if( OFStandard::dirExists(subdirectoryPath) )
  {
    return STATUS_Success;
  }
  else
  {
    size_t position = subdirectoryPath.rfind(PATH_SEPARATOR);
    if(position != OFString_npos)
    {
      OFString upperLevel = subdirectoryPath.substr(0, position);
      DIC_US mkResult = mkdirRecursive(upperLevel);
      if(mkResult != STATUS_Success)
      {
        return mkResult;
      }
      // else: upper level exist, create current level
    }

    // if it is not existent create it
    #ifdef HAVE_WINDOWS_H
    if( _mkdir( subdirectoryPath.c_str() ) == -1 )
    #else
    if( mkdir( subdirectoryPathAndName.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) == -1 )
    #endif
    {
      fprintf(stderr, "storescp: Could not create subdirectory %s.\n", subdirectoryPath.c_str() );
      return STATUS_STORE_Error_CannotUnderstand;
    }
    else
    {
      return STATUS_Success;
    }
  }
}
