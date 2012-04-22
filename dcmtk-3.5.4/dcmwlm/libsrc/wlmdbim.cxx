/*
*
*  Copyright (C) 1996-2005, OFFIS
*
*  This software and supporting documentation were developed by
*
*    Kuratorium OFFIS e.V.
*    Healthcare Information and Communication Systems
*    Escherweg 2
*    D-26121 Oldenburg, Germany
*
*  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
*  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
*  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
*  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
*  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
*
*  Module:  dcmwlm
*
*  Author:  Thomas Wilkens
*
*  Purpose: Class for managing file system interaction.
*
*  Last Update:      $Author: meichel $
*  Update Date:      $Date: 2005/12/08 15:48:34 $
*  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/libsrc/wlfsim.cc,v $
*  CVS/RCS Revision: $Revision: 1.16 $
*  Status:           $State: Exp $
*
*  CVS/RCS Log at end of file
*
*/

// ----------------------------------------------------------------------------

#include "dcmtk/dcmnet/diutil.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofoset.h"
#include "dcmtk/ofstd/ofsetit.h"
#include "dcmtk/ofstd/ofdate.h"
#include "dcmtk/ofstd/oftime.h"
#include "dcmtk/ofstd/oftypes.h"
#include "dcmtk/dcmnet/dicom.h"
#include "dcmtk/dcmdata/dcdatset.h"
#include "dcmtk/dcmdata/dcitem.h"
#include "dcmtk/dcmdata/dcvrda.h"
#include "dcmtk/dcmdata/dcvrtm.h"
#include "dcmtk/dcmwlm/wltypdef.h"
//#include "dcmtk/dcmdata/dctk.h"
//#include <stdio.h>
//#include <stdlib.h>
#include "dcmtk/dcmdata/dcvras.h"     // for DcmAgeString
#include "dcmtk/dcmdata/dcvrlo.h"     // for DcmLongString
#include "dcmtk/dcmdata/dcvrae.h"	  // for DcmApplicationEntity
#include "dcmtk/dcmdata/dcvrds.h"	  // for DcmDecimalString
#include "dcmtk/dcmdata/dcvrcs.h"	  // for DcmCodeString
#include "dcmtk/dcmdata/dcvrpn.h"	  // for DcmPersonName
#include "dcmtk/dcmdata/dcvrsh.h"	  // for DcmShortString
#include "dcmtk/dcmdata/dcdeftag.h"	  // for DCM_OffendingElement, ...
#include "dcmtk/dcmdata/dcsequen.h"	  // for DcmSequenceOfItems

#include "dcmtk/dcmwlm/wlmdbim.h"

// ----------------------------------------------------------------------------

WlmDBInteractionManager::WlmDBInteractionManager() 
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : Constructor.
// Parameters   : none.
// Return Value : none.
  : MaxMatchingDatasetCapability(0), matchingRecords(NULL)
{
}

// ----------------------------------------------------------------------------

WlmDBInteractionManager::~WlmDBInteractionManager(void)
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : Destructor.
// Parameters   : none.
// Return Value : none.
{
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::SetLogStream( OFConsole *value )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : Set value in member variable.
// Parameters   : value - [in] The value to set.
// Return Value : none.
{
  logStream = value;
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::SetVerbose( OFBool value )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : Set value in member variable.
// Parameters   : value - [in] The value to set.
// Return Value : none.
{
  verboseMode = value;
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::SetDebug( OFBool value )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : Set value in member variable.
// Parameters   : value - [in] The value to set.
// Return Value : none.
{
  debugMode = value;
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::SetEnableRejectionOfIncompleteWlFiles( OFBool value )
// Date         : May 3, 2005
// Author       : Thomas Wilkens
// Task         : Set value in member variable.
// Parameters   : value - [in] The value to set.
// Return Value : none.
{
  enableRejectionOfIncompleteWlFiles = value;
}

// ----------------------------------------------------------------------------
/*
OFCondition WlmFileSystemInteractionManager::ConnectToFileSystem( char *dfPathv )
*/
// ----------------------------------------------------------------------------
/*
OFCondition WlmFileSystemInteractionManager::DisconnectFromFileSystem()
*/
// ----------------------------------------------------------------------------

void WlmDBInteractionManager::DumpMessage( const char *message )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : This function dumps the given information on a stream.
//                Used for dumping information in normal, debug and verbose mode.
// Parameters   : message - [in] The message to dump.
// Return Value : none.
{
  if( logStream != NULL && message != NULL )
  {
    logStream->lockCout();
    logStream->getCout() << message << endl;
    logStream->unlockCout();
  }
}

// ----------------------------------------------------------------------------
/*
OFBool WlmFileSystemInteractionManager::IsCalledApplicationEntityTitleSupported( char *calledApplicationEntityTitlev )
*/
// ----------------------------------------------------------------------------

unsigned long WlmDBInteractionManager::DetermineMatchingRecords( DcmDataset *searchMask )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : This function determines the records from the worklist files which match
//                the given search mask and returns the number of matching records. Also,
//                this function will store the matching records in memory in the array
//                member variable matchingRecords.
// Parameters   : searchMask - [in] The search mask.
// Return Value : Number of matching records.
{
  char msg[128];

  // initialize member variables
  DcmDataset *dataset = NULL;
  numOfMatchingRecords = 0;

  StartGenerate(searchMask);
  // go through all records
  while(dataset = NextDataset(searchMask))
  {
    // in case option --enable-file-reject is set, we have to check if the current
    // .wl-file meets certain conditions; in detail, the file's dataset has to be
    // checked whether it contains all necessary return type 1 attributes and contains
    // information in all these attributes; if this is condition is not met, the
    // .wl-file shall be rejected
    if( enableRejectionOfIncompleteWlFiles && !DatasetIsComplete( dataset ) )
    {
      if( verboseMode )
      {
        sprintf( msg, "Record %d is incomplete. Record will be ignored.", numOfMatchingRecords );
        DumpMessage( msg );
      }
    }
    else
    {
      // check if the current dataset matches the matching key attribute values
      if( !DatasetMatchesSearchMask( dataset, searchMask ) )
      {
        if( verboseMode )
        {
          sprintf( msg, "Information from record %d does not match query.", numOfMatchingRecords );
          DumpMessage( msg );
        }
      }
      else
      {
        if( verboseMode )
        {
          sprintf( msg, "Information from record %d matches query.", numOfMatchingRecords );
          DumpMessage( msg );
        }

        // since the dataset matches the matching key attribute values
        // we need to insert it into the matchingRecords array
		if(numOfMatchingRecords >= MaxMatchingDatasetCapability)
		{
		  MaxMatchingDatasetCapability += CapabilityIncreaseStep;
          DcmDataset **tmp = new DcmDataset*[MaxMatchingDatasetCapability];
          for( unsigned long j=0 ; j<numOfMatchingRecords ; j++ )
            tmp[j] = matchingRecords[j];
          if(matchingRecords != NULL) delete[] matchingRecords;
          matchingRecords = tmp;
		}
		matchingRecords[numOfMatchingRecords] = dataset;

        ++numOfMatchingRecords;
      }
	}
  }
  StopGenerate();

  // return result
  return( numOfMatchingRecords );
}

// ----------------------------------------------------------------------------

unsigned long WlmDBInteractionManager::GetNumberOfSequenceItemsForMatchingRecord( DcmTagKey sequenceTag, WlmSuperiorSequenceInfoType *superiorSequenceArray, unsigned long numOfSuperiorSequences, unsigned long idx )
// Date         : January 6, 2004
// Author       : Thomas Wilkens
// Task         : For the matching record that is identified through idx, this function returns the number
//                of items that are contained in the sequence element that is referred to by sequenceTag.
//                In case this sequence element is itself contained in a certain item of another superior
//                sequence, superiorSequenceArray contains information about where to find the correct
//                sequence element.
// Parameters   : sequenceTag            - [in] The tag of the sequence element for which the number of items
//                                              shall be determined.
//                superiorSequenceArray  - [in] Array which contains information about superior sequence elements
//                                              the given sequence element is contained in.
//                numOfSuperiorSequences - [in] The number of elements in the above array.
//                idx                    - [in] Identifies the record from which the number of sequence items
//                                              shall be determined.
// Return Value : The number of items that are contained in the sequence element that is referred to by
//                sequenceTag and that can be found in sequence items which are specified in superiorSequenceArray.
{
  OFCondition cond;
  DcmSequenceOfItems *sequenceElement = NULL, *tmp = NULL;
  unsigned long i;

  // initialize result variable
  unsigned long numOfItems = 0;

  // if the sequence in question is not contained in another sequence
  if( numOfSuperiorSequences == 0 )
  {
    // simply find and get this sequence in the matching dataset (on the main level)
    cond = matchingRecords[idx]->findAndGetSequence( sequenceTag, sequenceElement, OFFalse );
  }
  else
  {
    // if it is contained in (an)other sequence(s), find it
    cond = matchingRecords[idx]->findAndGetSequence( superiorSequenceArray[0].sequenceTag, sequenceElement, OFFalse );
    for( i=1 ; i<numOfSuperiorSequences && cond.good() ; i++ )
    {
      cond = sequenceElement->getItem( superiorSequenceArray[i-1].currentItem )->findAndGetSequence( superiorSequenceArray[i].sequenceTag, tmp, OFFalse );
      if( cond.good() )
        sequenceElement = tmp;
    }

    if( cond.good() )
    {
      cond = sequenceElement->getItem( superiorSequenceArray[ numOfSuperiorSequences - 1 ].currentItem )->findAndGetSequence( sequenceTag, tmp, OFFalse );
      if( cond.good() )
        sequenceElement = tmp;
    }
  }

  // determine number of items for the sequence in question
  if( cond.good() )
    numOfItems = sequenceElement->card();

  // return result
  return( numOfItems );
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::GetAttributeValueForMatchingRecord(DcmTagKey tag, WlmSuperiorSequenceInfoType *superiorSequenceArray, unsigned long numOfSuperiorSequences, unsigned long idx, char *&value )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : This function determines an attribute value of a matching record
//                and returns this value in a newly created string to the caller.
// Parameters   : tag                    - [in] Attribute tag. Specifies which attribute's value shall be returned.
//                superiorSequenceArray  - [in] Array which contains information about superior sequence elements
//                                              the given element is contained in.
//                numOfSuperiorSequences - [in] The number of elements in the above array.
//                idx                    - [in] Identifies the record from which the attribute value shall be retrieved.
//                value                  - [out] Pointer to a newly created string that contains the requested value.
//                                               If value was not found an emtpy string will be returned.
// Return Value : none.
{
  OFCondition cond;
  DcmSequenceOfItems *sequenceElement = NULL, *tmp = NULL;
  unsigned long i;
  const char *val = NULL;
  Uint16 v;

  // if the element in question is not contained in another sequence
  if( numOfSuperiorSequences == 0 )
  {
    // simply find and get this element in the matching dataset (on the main level);
    // here, we have to distinguish two cases: attribute PregnancyStatus has to be re-
    // trieved as a Uint16 value (also note for this case, that this attribute can only
    // occur on the main level, it will never be contained in a sequence), all other
    // attributes have to be retrieved as strings
    if( tag == DCM_PregnancyStatus )
    {
      cond = matchingRecords[idx]->findAndGetUint16( tag, v, 0, OFFalse );
      if( cond.good() )
      {
        value = new char[20];
        sprintf( value, "%u", v );
      }
      else
      {
        value = new char[ 2 ];
        strcpy( value, "4" );           // a value of "4" in attribute PregnancyStatus means "unknown" in DICOM
      }
    }
    else
    {
      cond = matchingRecords[idx]->findAndGetString( tag, val, OFFalse );
      if( cond.good() && val != NULL )
      {
        value = new char[ strlen( val ) + 1 ];
        strcpy( value, val );
      }
      else
      {
        value = new char[ 1 ];
        strcpy( value, "" );
      }
    }
  }
  else
  {
    // if it is contained in (an)other sequence(s), go through all corresponding sequence items
    cond = matchingRecords[idx]->findAndGetSequence( superiorSequenceArray[0].sequenceTag, sequenceElement, OFFalse );
    for( i=1 ; i<numOfSuperiorSequences && cond.good() ; i++ )
    {
      cond = sequenceElement->getItem( superiorSequenceArray[i-1].currentItem )->findAndGetSequence( superiorSequenceArray[i].sequenceTag, tmp, OFFalse );
      if( cond.good() )
        sequenceElement = tmp;
    }

    // now sequenceElement points to the sequence element in which the attribute
    // in question can be found; retrieve a value for this attribute (note that
    // all attributes in sequences can actually be retrieved as strings)
    if( cond.good() )
    {
      cond = sequenceElement->getItem( superiorSequenceArray[ numOfSuperiorSequences - 1 ].currentItem )->findAndGetString( tag, val, OFFalse );
      if( cond.good() && val != NULL )
      {
        value = new char[ strlen( val ) + 1 ];
        strcpy( value, val );
      }
      else
      {
        value = new char[ 1 ];
        strcpy( value, "" );
      }
    }
    else
    {
      value = new char[ 1 ];
      strcpy( value, "" );
    }
  }
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::ClearMatchingRecords()
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : This function frees the memory which was occupied by matchingRecords.
//                It shall be called when the matching records are no longer needed.
// Parameters   : none.
// Return Value : none.
{
  for( unsigned int i=0 ; i<numOfMatchingRecords ; i++ )
    delete matchingRecords[i];
  if(matchingRecords != NULL) delete[] matchingRecords;
  matchingRecords = NULL;
  numOfMatchingRecords = 0;
  MaxMatchingDatasetCapability = 0;
}

// ----------------------------------------------------------------------------
/*
void WlmFileSystemInteractionManager::DetermineWorklistFiles( OFOrderedSet<OFString> &worklistFiles )
*/
// ----------------------------------------------------------------------------
/*
OFBool WlmFileSystemInteractionManager::IsWorklistFile( const char *fname )
*/
// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DatasetIsComplete( DcmDataset *dataset )
// Date         : May 2, 2005
// Author       : Thomas Wilkens
// Task         : This function checks if the given dataset (which represents the information from a
//                worklist file) contains all necessary return type 1 information. According to the
//                DICOM standard part 4 annex K, the following attributes are type 1 attributes in
//                C-Find RSP messages:
//                      Attribute                             Tag      Return Key Type
//                  SpecificCharacterSet                  (0008,0005)        1C (will be checked in WlmDataSourceFileSystem::StartFindRequest(...); this attribute does not have to be checked here)
//                  ScheduledProcedureStepSequence        (0040,0100)        1
//                   > ScheduledStationAETitle            (0040,0001)        1
//                   > ScheduledProcedureStepStartDate    (0040,0002)        1
//                   > ScheduledProcedureStepStartTime    (0040,0003)        1
//                   > Modality                           (0008,0060)        1
//                   > ScheduledProcedureStepDescription  (0040,0007)        1C (The ScheduledProcedureStepDescription (0040,0007) or the ScheduledProtocolCodeSequence (0040,0008) or both shall be supported by the SCP; we actually support both, so we have to check if at least one of the two attributes contains valid information.)
//                   > ScheduledProtocolCodeSequence      (0040,0008)        1C (see abobve)
//                   > > CodeValue                        (0008,0100)        1
//                   > > CodingSchemeDesignator           (0008,0102)        1
//                   > ScheduledProcedureStepID           (0040,0009)        1
//                  RequestedProcedureID                  (0040,1001)        1
//                  RequestedProcedureDescription         (0032,1060)        1C (The RequestedProcedureDescription (0032,1060) or the RequestedProcedureCodeSequence (0032,1064) or both shall be supported by the SCP; we actually support both, so we have to check if at least one of the two attributes contains valid information.)
//                  RequestedProcedureCodeSequence        (0032,1064)        1C (see abobve)
//                   > > CodeValue                        (0008,0100)        1
//                   > > CodingSchemeDesignator           (0008,0102)        1
//                  StudyInstanceUID                      (0020,000D)        1
//                  ReferencedStudySequence               (0008,1110)        2
//                   > ReferencedSOPClassUID              (0008,1150)        1C (Required if a sequence item is present)
//                   > ReferencedSOPInstanceUID           (0008,1155)        1C (Required if a sequence item is present)
//                  ReferencedPatientSequence             (0008,1120)        2
//                   > ReferencedSOPClassUID              (0008,1150)        1C (Required if a sequence item is present)
//                   > ReferencedSOPInstanceUID           (0008,1155)        1C (Required if a sequence item is present)
//                  PatientsName                          (0010,0010)        1
//                  PatientID                             (0010,0020)        1
// Parameters   : dataset - [in] The dataset of the worklist file which is currently examined.
// Return Value : OFTrue in case the given dataset contains all necessary return type 1 information,
//                OFFalse otherwise.
{
  DcmElement *scheduledProcedureStepSequence = NULL;

  // intialize returnValue
  OFBool complete = OFTrue;

  // the dataset is considered to be incomplete...
  // ...if the ScheduledProcedureStepSequence is missing or
  // ...if the ScheduledProcedureStepSequence does not have exactly one item
  if( dataset->findAndGetElement( DCM_ScheduledProcedureStepSequence, scheduledProcedureStepSequence ).bad() || ((DcmSequenceOfItems*)scheduledProcedureStepSequence)->card() != 1 )
    complete = OFFalse;
  else
  {
    // so the ScheduledProcedureStepSequence is existent and has exactly one item;
    // get this one and only item from the ScheduledProcedureStepSequence
    DcmItem *scheduledProcedureStepSequenceItem = ((DcmSequenceOfItems*)scheduledProcedureStepSequence)->getItem(0);

    // the dataset is considered to be incomplete...
    // ...if ScheduledStationAETitle is missing or empty in the ScheduledProcedureStepSequence, or
    // ...if ScheduledProcedureStepStartDate is missing or empty in the ScheduledProcedureStepSequence, or
    // ...if ScheduledProcedureStepStartTime is missing or empty in the ScheduledProcedureStepSequence, or
    // ...if Modality is missing or empty in the ScheduledProcedureStepSequence, or
    // ...if ScheduledProcedureStepID is missing or empty in the ScheduledProcedureStepSequence, or
    // ...if RequestedProcedureID is missing or empty in the dataset, or
    // ...if StudyInstanceUID is missing or empty in the dataset, or
    // ...if PatientsName is missing or empty in the dataset, or
    // ...if PatientID is missing or empty in the dataset, or
    // ...if inside the ScheduledProcedureStepSequence, no information can be retrieved from
    //    the ScheduledProcedureStepDescription attribute and (at the same time) also no
    //    complete information can be retrieved from the ScheduledProtocolCodeSequence attribute, or
    // ...if inside the dataset, no information can be retrieved from the RequestedProcedureDescription
    //    attribute and (at the same time) also no complete information can be retrieved from the
    //    RequestedProcedureCodeSequence attribute, or
    // ...if inside the dataset, the ReferencedStudySequence is either absent or it is existent and non-empty but incomplete, or
    // ...if inside the dataset, the ReferencedPatientSequence is either absent or it is existent and non-empty but incomplete
    if( AttributeIsAbsentOrEmpty( DCM_ScheduledStationAETitle, scheduledProcedureStepSequenceItem ) ||
        AttributeIsAbsentOrEmpty( DCM_ScheduledProcedureStepStartDate, scheduledProcedureStepSequenceItem ) ||
        AttributeIsAbsentOrEmpty( DCM_ScheduledProcedureStepStartTime, scheduledProcedureStepSequenceItem ) ||
        AttributeIsAbsentOrEmpty( DCM_Modality, scheduledProcedureStepSequenceItem ) ||
        AttributeIsAbsentOrEmpty( DCM_ScheduledProcedureStepID, scheduledProcedureStepSequenceItem ) ||
        AttributeIsAbsentOrEmpty( DCM_RequestedProcedureID, dataset ) ||
        AttributeIsAbsentOrEmpty( DCM_StudyInstanceUID, dataset ) ||
        AttributeIsAbsentOrEmpty( DCM_PatientsName, dataset ) ||
        AttributeIsAbsentOrEmpty( DCM_PatientID, dataset ) ||
        DescriptionAndCodeSequenceAttributesAreIncomplete( DCM_ScheduledProcedureStepDescription, DCM_ScheduledProtocolCodeSequence, scheduledProcedureStepSequenceItem ) ||
        DescriptionAndCodeSequenceAttributesAreIncomplete( DCM_RequestedProcedureDescription, DCM_RequestedProcedureCodeSequence, dataset ) ||
        ReferencedStudyOrPatientSequenceIsAbsentOrExistentButNonEmptyAndIncomplete( DCM_ReferencedStudySequence, dataset ) ||
        ReferencedStudyOrPatientSequenceIsAbsentOrExistentButNonEmptyAndIncomplete( DCM_ReferencedPatientSequence, dataset ) )
      complete = OFFalse;
  }

  // return result
  return( complete );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ReferencedStudyOrPatientSequenceIsAbsentOrExistentButNonEmptyAndIncomplete( DcmTagKey sequenceTagKey, DcmItem *dset )
// Date         : May 4, 2005
// Author       : Thomas Wilkens
// Task         : This function checks if the specified sequence attribute is absent or existent but non-empty
//                and incomplete in the given dataset.
// Parameters   : sequenceTagKey - [in] The sequence attribute which shall be checked.
//                dset           - [in] The dataset in which the attribute is contained.
// Return Value : OFTrue in case the sequence attribute is absent or existent but non-empty and incomplete, OFFalse otherwise.
{
  DcmElement *sequence = NULL;
  OFBool result;

  // if the sequence attribute is absent, we want to return OFTrue
  if( dset->findAndGetElement( sequenceTagKey, sequence ).bad() )
    result = OFTrue;
  else
  {
    // if the sequence attribute is existent but empty, we want to return OFFalse
    // (note that the sequence is actually type 2, so being empty is ok)
    if( ((DcmSequenceOfItems*)sequence)->card() == 0 )
      result = OFFalse;
    else
    {
      // if the sequence attribute is existent and non-empty, we need
      // to check every item in the sequence for completeness
      result = OFFalse;
      for( unsigned long i=0 ; i<((DcmSequenceOfItems*)sequence)->card() && !result ; i++ )
      {
        if( AttributeIsAbsentOrEmpty( DCM_ReferencedSOPClassUID, ((DcmSequenceOfItems*)sequence)->getItem(i) ) ||
            AttributeIsAbsentOrEmpty( DCM_ReferencedSOPInstanceUID, ((DcmSequenceOfItems*)sequence)->getItem(i) ) )
          result = OFTrue;
      }
    }
  }

  // return result
  return( result );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DescriptionAndCodeSequenceAttributesAreIncomplete( DcmTagKey descriptionTagKey, DcmTagKey codeSequenceTagKey, DcmItem *dset )
// Date         : May 4, 2005
// Author       : Thomas Wilkens
// Task         : This function checks if the specified description and code sequence attribute are both incomplete in the given dataset.
// Parameters   : descriptionTagKey  - [in] The description attribute which shall be checked.
//                codeSequenceTagKey - [in] The codeSequence attribute which shall be checked.
//                dset               - [in] The dataset in which the attributes are contained.
// Return Value : OFTrue in case both attributes are incomplete, OFFalse otherwise.
{
  DcmElement *codeSequence = NULL;

  // check if the code sequence attribute is complete,
  // i.e. if complete information can be retrieved from this attribute

  // if the attribute is not existent or has no items, we consider it incomplete
  OFBool codeSequenceComplete = OFTrue;
  if( dset->findAndGetElement( codeSequenceTagKey, codeSequence ).bad() || ((DcmSequenceOfItems*)codeSequence)->card() == 0 )
    codeSequenceComplete = OFFalse;
  else
  {
    // if it is existent and has items, check every item for completeness
    for( unsigned long i=0 ; i<((DcmSequenceOfItems*)codeSequence)->card() && codeSequenceComplete ; i++ )
    {
      if( AttributeIsAbsentOrEmpty( DCM_CodeValue, ((DcmSequenceOfItems*)codeSequence)->getItem(i) ) ||
          AttributeIsAbsentOrEmpty( DCM_CodingSchemeDesignator, ((DcmSequenceOfItems*)codeSequence)->getItem(i) ) )
        codeSequenceComplete = OFFalse;
    }
  }

  // now check the above condition
  if( !codeSequenceComplete && AttributeIsAbsentOrEmpty( descriptionTagKey, dset ) )
    return( OFTrue );
  else
    return( OFFalse );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::AttributeIsAbsentOrEmpty( DcmTagKey elemTagKey, DcmItem *dset )
// Date         : May 4, 2005
// Author       : Thomas Wilkens
// Task         : This function checks if the specified attribute is absent or contains an empty value in the given dataset.
// Parameters   : elemTagKey - [in] The attribute which shall be checked.
//                dset       - [in] The dataset in which the attribute is contained.
// Return Value : OFTrue in case the attribute is absent or contains an empty value, OFFalse otherwise.
{
  DcmElement *elem = NULL;

  if( dset->findAndGetElement( elemTagKey, elem ).bad() || elem->getLength() == 0 )
    return( OFTrue );
  else
    return( OFFalse );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DatasetMatchesSearchMask( DcmDataset *dataset, DcmDataset *searchMask )
// Date         : July 11, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue, if the matching key attribute values in the
//                dataset match the matching key attribute values in the search mask.
// Parameters   : dataset    - [in] The dataset which shall be checked.
//                searchMask - [in] The search mask.
// Return Value : OFTrue  - The dataset matches the search mask in the matching key attribute values.
//                OFFalse - The dataset does not match the search mask in the matching key attribute values.
{
  OFBool dateTimeMatchHasBeenPerformed = OFFalse;

  // initialize result variable
  OFBool matchFound = OFTrue;

  // determine matching key attribute values in the dataset
  const char **mkaValuesDataset = NULL;
  DetermineMatchingKeyAttributeValues( dataset, mkaValuesDataset );

  // determine matching key attribute values in the search mask
  const char **mkaValuesSearchMask = NULL;
  DetermineMatchingKeyAttributeValues( searchMask, mkaValuesSearchMask );

  // go through the arrays of matching key attribute values
  for( unsigned long i=0 ; i<NUMBER_OF_SUPPORTED_MATCHING_KEY_ATTRIBUTES && matchFound ; i++ )
  {
    // check if the current matching key attribute actually had a value in the search mask
    if( mkaValuesSearchMask[i] != NULL  )
    {
      // if it did, check if the values from search mask and dataset match
      switch( i )
      {
        case 0:
          // matching key attribute is DCM_ScheduledStationAETitle (AE, 1-n)
          matchFound = ScheduledStationAETitlesMatch( mkaValuesDataset[0], mkaValuesSearchMask[0] );
          break;

        case 1:
        case 2:
          // matching key attributes are DCM_ScheduledProcedureStepStartDate (DA, 1)
          // and DCM_ScheduledProcedureStepStartTime (TM, 1)
          // only do something if a date time match has not yet been performed
          if( !dateTimeMatchHasBeenPerformed )
          {
            matchFound = ScheduledProcedureStepStartDateTimesMatch( mkaValuesDataset[1], mkaValuesDataset[2], mkaValuesSearchMask[1], mkaValuesSearchMask[2] );
            dateTimeMatchHasBeenPerformed = OFTrue;
          }
          break;

        case 3:
          // matching key attribute is DCM_Modality (CS, 1)
          matchFound = ModalitiesMatch( mkaValuesDataset[3], mkaValuesSearchMask[3] );
          break;

        case 4:
          // matching key attribute is DCM_ScheduledPerformingPhysiciansName (PN, 1)
          matchFound = ScheduledPerformingPhysiciansNamesMatch( mkaValuesDataset[4], mkaValuesSearchMask[4] );
          break;

        case 5:
          // matching key attribute is DCM_PatientsName (PN, 1)
          matchFound = PatientsNamesMatch( mkaValuesDataset[5], mkaValuesSearchMask[5] );
          break;

        case 6:
          // matching key attribute is DCM_PatientID (LO, 1)
          matchFound = PatientIdsMatch( mkaValuesDataset[6], mkaValuesSearchMask[6] );
          break;

        case 7:
          // matching key attribute is DCM_AccessionNumber (SH, 2)
          matchFound = AccessionNumbersMatch( mkaValuesDataset[7], mkaValuesSearchMask[7] );
          break;

        case 8:
          // matching key attribute is DCM_RequestedProcedureID (SH, 1)
          matchFound = RequestedProcedureIdsMatch( mkaValuesDataset[8], mkaValuesSearchMask[8] );
          break;

        case 9:
          // matching key attribute is DCM_ReferringPhysiciansName (PN, 2)
          matchFound = ReferringPhysiciansNamesMatch( mkaValuesDataset[9], mkaValuesSearchMask[9] );
          break;

        case 10:
          // matching key attribute is DCM_PatientsSex (CS, 2)
          matchFound = PatientsSexesMatch( mkaValuesDataset[10], mkaValuesSearchMask[10] );
          break;

        case 11:
          // matching key attribute is DCM_RequestingPhysician (PN, 2)
          matchFound = RequestingPhysiciansMatch( mkaValuesDataset[11], mkaValuesSearchMask[11] );
          break;

        case 12:
          // matching key attribute is DCM_AdmissionID (LO, 2)
          matchFound = AdmissionIdsMatch( mkaValuesDataset[12], mkaValuesSearchMask[12] );
          break;

        case 13:
          // matching key attribute is DCM_RequestedProcedurePriority (SH, 2)
          matchFound = RequestedProcedurePrioritiesMatch( mkaValuesDataset[13], mkaValuesSearchMask[13] );
          break;

        case 14:
          // matching key attribute is DCM_PatientsBirthDate (DA, 2)
          matchFound = PatientsBirthDatesMatch( mkaValuesDataset[14], mkaValuesSearchMask[14] );
          break;

        default:
          break;
      }
    }
  }

  // return result
  return( matchFound );
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::DetermineMatchingKeyAttributeValues( DcmDataset *dataset, const char **&matchingKeyAttrValues )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function determines the values of the matching key attributes in the given dataset.
// Parameters   : dataset               - [in] Dataset from which the values shall be extracted.
//                matchingKeyAttrValues - [out] Contains in the end the values of the matching key
//                                        attributes in the search mask. Is an array of pointers.
// Return Value : none.
{
  // Initialize array of strings because all currently
  // supported matching key attributes are strings
  matchingKeyAttrValues = new const char*[ NUMBER_OF_SUPPORTED_MATCHING_KEY_ATTRIBUTES ];

  // find matching key attributes in the dataset and remember their values.
  for( unsigned long i=0 ; i<NUMBER_OF_SUPPORTED_MATCHING_KEY_ATTRIBUTES ; i++ )
  {
    // initialize array field
    matchingKeyAttrValues[i] = NULL;

    // determine which matching key attribute we want to find
    DcmTagKey tag;
    switch( i )
    {
      case 0 : tag = DCM_ScheduledStationAETitle           ; break;
      case 1 : tag = DCM_ScheduledProcedureStepStartDate   ; break;
      case 2 : tag = DCM_ScheduledProcedureStepStartTime   ; break;
      case 3 : tag = DCM_Modality                          ; break;
      case 4 : tag = DCM_ScheduledPerformingPhysiciansName ; break;
      case 5 : tag = DCM_PatientsName                      ; break;
      case 6 : tag = DCM_PatientID                         ; break;
      case 7 : tag = DCM_AccessionNumber                   ; break;
      case 8 : tag = DCM_RequestedProcedureID              ; break;
      case 9 : tag = DCM_ReferringPhysiciansName           ; break;
      case 10 : tag = DCM_PatientsSex                      ; break;
      case 11 : tag = DCM_RequestingPhysician              ; break;
      case 12 : tag = DCM_AdmissionID                      ; break;
      case 13 : tag = DCM_RequestedProcedurePriority       ; break;
      case 14 : tag = DCM_PatientsBirthDate                ; break;
      default:                                               break;
    }

    // try to find matching key attribute in the dataset
    dataset->findAndGetString( tag, matchingKeyAttrValues[i], OFTrue );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ScheduledStationAETitlesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute scheduled station AE title match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // if there is a value in the dataset, perform case sensitive single value matching
  if( datasetValue != NULL )
    return( CaseSensitiveSingleValueMatch( datasetValue, searchMaskValue ) );
  else
  {
    // if datasetValue is not existent, the search mask's value has to be empty to have
    // a match (universal matching); in all other cases, we do not have a match
    if( strcmp( searchMaskValue, "" ) == 0 )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ScheduledProcedureStepStartDateTimesMatch( const char *datasetDateValue, const char *datasetTimeValue, const char *searchMaskDateValue, const char *searchMaskTimeValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attributes scheduled procedure step start date and scheduled procedure step
//                start time match; otherwise OFFalse will be returned.
// Parameters   : datasetDateValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                datasetTimeValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskDateValue - [in] Value for the corresponding attribute in the search mask; might be NULL.
//                searchMaskTimeValue - [in] Value for the corresponding attribute in the search mask; might be NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  OFBool isMatch = OFFalse;

  // four things are of interest before matching
  OFBool dateGiven = ( searchMaskDateValue != NULL ) ? OFTrue : OFFalse;
  OFBool timeGiven = ( searchMaskTimeValue != NULL ) ? OFTrue : OFFalse;
  OFBool dateIsDateRange = ( dateGiven && strchr( searchMaskDateValue, '-' ) != NULL ) ? OFTrue : OFFalse;
  OFBool timeIsTimeRange = ( timeGiven && strchr( searchMaskTimeValue, '-' ) != NULL ) ? OFTrue : OFFalse;

  // depending on these four things perform different kinds of matching
  if( dateIsDateRange && timeIsTimeRange )
  {
    // perform range matching taking into account date and time
    isMatch = DateTimeRangeMatch( datasetDateValue, datasetTimeValue, searchMaskDateValue, searchMaskTimeValue );
  }
  else if( dateIsDateRange && !timeIsTimeRange )
  {
    // perform range matching taking into account date only
    isMatch = DateRangeMatch( datasetDateValue, searchMaskDateValue );
  }
  else if( dateGiven && !dateIsDateRange && timeIsTimeRange )
  {
    // perform a date single value match and a time range match
    isMatch = DateSingleValueMatch( datasetDateValue, searchMaskDateValue ) && TimeRangeMatch( datasetTimeValue, searchMaskTimeValue );
  }
  else if( !dateGiven && !dateIsDateRange && timeIsTimeRange )
  {
    // perform range matching taking into account time only
    isMatch = TimeRangeMatch( datasetTimeValue, searchMaskTimeValue );
  }
  else if( dateGiven && !dateIsDateRange && timeGiven && !timeIsTimeRange )
  {
    // perform single value matching taking into account date and time
    isMatch = DateTimeSingleValueMatch( datasetDateValue, datasetTimeValue, searchMaskDateValue, searchMaskTimeValue );
  }
  else if( dateGiven && !dateIsDateRange && !timeGiven && !timeIsTimeRange )
  {
    // perform single value matching taking into account date only
    isMatch = DateSingleValueMatch( datasetDateValue, searchMaskDateValue );
  }
  else if( !dateGiven && timeGiven && !timeIsTimeRange && !dateIsDateRange )
  {
    // perform single value matching taking into account time only
    isMatch = TimeSingleValueMatch( datasetTimeValue, searchMaskTimeValue );
  }
  else // if( !dateGiven && !timeGiven && !dateIsDateRange && !timeIsTimeRange ) // this case can actually never happen, because either time or date will always be given in this function
  {
    // return OFTrue
    isMatch = OFTrue;
  }

  // return result
  return( isMatch );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ModalitiesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute modality match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // if there is a value in the dataset, perform case sensitive single value matching
  if( datasetValue != NULL )
    return( CaseSensitiveSingleValueMatch( datasetValue, searchMaskValue ) );
  else
  {
    // if datasetValue is not existent, the search mask's value has to be empty to have
    // a match (universal matching); in all other cases, we do not have a match
    if( strcmp( searchMaskValue, "" ) == 0 )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ScheduledPerformingPhysiciansNamesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute scheduled performing physician's names match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we have to perform case sensitive single value matching or wildcard matching
    else if( CaseSensitiveSingleValueMatch( datasetValue, searchMaskValue ) || WildcardMatch( datasetValue, searchMaskValue ) )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::PatientsNamesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute patient's names match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we have to perform case sensitive single value matching or wildcard matching
    else if( CaseSensitiveSingleValueMatch( datasetValue, searchMaskValue ) || WildcardMatch( datasetValue, searchMaskValue ) )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::PatientIdsMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute patient id match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // if there is a value in the dataset, perform case sensitive single value matching
  if( datasetValue != NULL )
    return( CaseSensitiveSingleValueMatch( datasetValue, searchMaskValue ) );
  else
  {
    // if datasetValue is not existent, the search mask's value has to be empty to have
    // a match (universal matching); in all other cases, we do not have a match
    if( strcmp( searchMaskValue, "" ) == 0 )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::AccessionNumbersMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute accession number match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::RequestedProcedureIdsMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute requested procedure id match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::ReferringPhysiciansNamesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute referring physician's name match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::PatientsSexesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute patient sex match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::RequestingPhysiciansMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute requesting physician match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::AdmissionIdsMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute admission id match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::RequestedProcedurePrioritiesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : December 22, 2003
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute requested procedure priorities match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // copy search mask value
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  sv = DU_stripTrailingSpaces( sv );

  // if we are dealing with universal matching, there is always a match
  if( strcmp( sv, "" ) == 0 || strcmp( sv, "*" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching...
    // ...and the dataset value is not existent, there is no match
    if( datasetValue == NULL )
      return( OFFalse );
    // ...and the dataset value is existent, we want to perform wildcard matching
    else
      return( WildcardMatch( datasetValue, searchMaskValue ) );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::PatientsBirthDatesMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : September 23, 2005
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values in
//                attribute patient's birth date match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  OFBool isMatch = OFFalse;

  // before matching, determine if the date value is a date range value
  OFBool dateIsDateRange = ( strchr( searchMaskValue, '-' ) != NULL ) ? OFTrue : OFFalse;

  // depending upon this information, perform different kinds of matching
  if( dateIsDateRange )
  {
    // perform range matching
    isMatch = DateRangeMatch( datasetValue, searchMaskValue );
  }
  else
  {
    // perform single value matching
    isMatch = DateSingleValueMatch( datasetValue, searchMaskValue );
  }

  // return result
  return( isMatch );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DateTimeRangeMatch( const char *datasetDateValue, const char *datasetTimeValue, const char *searchMaskDateValue, const char *searchMaskTimeValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a date time range match and returns OFTrue if the dataset's
//                and the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetDateValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                datasetTimeValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskDateValue - [in] Value for the corresponding attribute in the search mask; never NULL.
//                searchMaskTimeValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  char *sdvv1 = NULL, *sdvv2 = NULL, *stvv1 = NULL, *stvv2 = NULL;
  OFDate datasetDateVal, searchMaskDateVal1, searchMaskDateVal2;
  OFTime datasetTimeVal, searchMaskTimeVal1, searchMaskTimeVal2;
  OFCondition cond;

  // if values in dataset are both given, perform date time range match
  if( datasetDateValue != NULL && datasetTimeValue != NULL )
  {
    // initialize result
    OFBool isMatch = OFFalse;

    // copy values
    char *ddv = new char[ strlen( datasetDateValue ) + 1 ];     strcpy( ddv, datasetDateValue );
    char *dtv = new char[ strlen( datasetTimeValue ) + 1 ];     strcpy( dtv, datasetTimeValue );
    char *sdv = new char[ strlen( searchMaskDateValue ) + 1 ];  strcpy( sdv, searchMaskDateValue );
    char *stv = new char[ strlen( searchMaskTimeValue ) + 1 ];  strcpy( stv, searchMaskTimeValue );

    // strip trailing spaces
    ddv = DU_stripTrailingSpaces( ddv );
    dtv = DU_stripTrailingSpaces( dtv );
    sdv = DU_stripTrailingSpaces( sdv );
    stv = DU_stripTrailingSpaces( stv );

    // get actual date/time boundary values
    ExtractValuesFromRange( sdv, sdvv1, sdvv2 );
    ExtractValuesFromRange( stv, stvv1, stvv2 );

    // generate OFDate and OFTime objects from strings
    cond = DcmDate::getOFDateFromString( OFString( ddv ), datasetDateVal );
    if( cond.good() )
    {
      cond = DcmTime::getOFTimeFromString( OFString( dtv ), datasetTimeVal );
      if( cond.good() )
      {
        cond = DcmDate::getOFDateFromString( (( sdvv1 != NULL ) ? OFString( sdvv1 ) : OFString("19000101")), searchMaskDateVal1 );
        if( cond.good() )
        {
          cond = DcmDate::getOFDateFromString( (( sdvv2 != NULL ) ? OFString( sdvv2 ) : OFString("39991231")), searchMaskDateVal2 );
          if( cond.good() )
          {
            cond = DcmTime::getOFTimeFromString( (( stvv1 != NULL ) ? OFString( stvv1 ) : OFString("000000")), searchMaskTimeVal1 );
            if( cond.good() )
            {
              cond = DcmTime::getOFTimeFromString( (( stvv2 != NULL ) ? OFString( stvv2 ) : OFString("235959")), searchMaskTimeVal2 );
              if( cond.good() )
              {
                // now that we have the date and time objects we can actually
                // compare the date and time values and determine if it's a match

                // check if the lower value in the search mask is earlier
                // than or equal to the lower value in the dataset
                if( searchMaskDateVal1 < datasetDateVal ||
                    ( searchMaskDateVal1 == datasetDateVal && searchMaskTimeVal1 <= datasetTimeVal ) )
                {
                  // now check if the upper value in the search mask is later
                  // than or equal to the upper value in the dataset
                  if( searchMaskDateVal2 > datasetDateVal ||
                      ( searchMaskDateVal2 == datasetDateVal && searchMaskTimeVal2 >= datasetTimeVal ) )
                  {
                    // if all these conditions are met, then this is considered to be a match
                    isMatch = OFTrue;
                  }
                }
              }
            }
          }
        }
      }
    }

    // free memory
    delete ddv;
    delete dtv;
    delete sdv;
    delete stv;
    if( sdvv1 != NULL ) delete sdvv1;
    if( sdvv2 != NULL ) delete sdvv2;
    if( stvv1 != NULL ) delete stvv1;
    if( stvv2 != NULL ) delete stvv2;

    // return result
    return( isMatch );
  }
  else
  {
    // so at least one of the two values (date or time) or not given in
    // the dataset; in this case, since the date and time values in the
    // search mask are both non-universal, we do not have a match
    return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DateRangeMatch( const char *datasetDateValue, const char *searchMaskDateValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a date range match and returns OFTrue if the dataset's and
//                the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetDateValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskDateValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  char *sdvv1 = NULL, *sdvv2 = NULL;
  OFDate datasetDateVal, searchMaskDateVal1, searchMaskDateVal2;
  OFCondition cond;

  // if date value in dataset is given, perform date range match
  if( datasetDateValue != NULL )
  {
    // initialize result
    OFBool isMatch = OFFalse;

    // copy values
    char *ddv = new char[ strlen( datasetDateValue ) + 1 ];     strcpy( ddv, datasetDateValue );
    char *sdv = new char[ strlen( searchMaskDateValue ) + 1 ];  strcpy( sdv, searchMaskDateValue );

    // strip trailing spaces
    ddv = DU_stripTrailingSpaces( ddv );
    sdv = DU_stripTrailingSpaces( sdv );

    // get actual date boundary values
    ExtractValuesFromRange( sdv, sdvv1, sdvv2 );

    // generate OFDate objects from strings
    cond = DcmDate::getOFDateFromString( OFString( ddv ), datasetDateVal );
    if( cond.good() )
    {
      cond = DcmDate::getOFDateFromString( (( sdvv1 != NULL ) ? OFString( sdvv1 ) : OFString("19000101")), searchMaskDateVal1 );
      if( cond.good() )
      {
        cond = DcmDate::getOFDateFromString( (( sdvv2 != NULL ) ? OFString( sdvv2 ) : OFString("39991231")), searchMaskDateVal2 );
        if( cond.good() )
        {
          // now that we have the date objects we can actually
          // compare the date values and determine if it's a match

          // check if the lower value in the search mask is earlier
          // than or equal to the lower value in the dataset
          if( searchMaskDateVal1 <= datasetDateVal )
          {
            // now check if the upper value in the search mask is later
            // than or equal to the upper value in the dataset
            if( searchMaskDateVal2 >= datasetDateVal )
            {
              // if these conditions are met, then this is considered to be a match
              isMatch = OFTrue;
            }
          }
        }
      }
    }

    // free memory
    delete ddv;
    delete sdv;
    if( sdvv1 != NULL ) delete sdvv1;
    if( sdvv2 != NULL ) delete sdvv2;

    // return result
    return( isMatch );
  }
  else
  {
    // so dataset date value is not given; in this case, since the date
    // value in the search mask is non-universal, we do not have a match
    return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::TimeRangeMatch( const char *datasetTimeValue, const char *searchMaskTimeValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a time range match and returns OFTrue if the dataset's and
//                the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetTimeValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskTimeValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  char *stvv1 = NULL, *stvv2 = NULL;
  OFTime datasetTimeVal, searchMaskTimeVal1, searchMaskTimeVal2;
  OFCondition cond;

  // if time value in dataset is given, perform date range match
  if( datasetTimeValue != NULL )
  {
    // initialize result
    OFBool isMatch = OFFalse;

    // copy values
    char *dtv = new char[ strlen( datasetTimeValue ) + 1 ];     strcpy( dtv, datasetTimeValue );
    char *stv = new char[ strlen( searchMaskTimeValue ) + 1 ];  strcpy( stv, searchMaskTimeValue );

    // strip trailing spaces
    dtv = DU_stripTrailingSpaces( dtv );
    stv = DU_stripTrailingSpaces( stv );

    // get actual time boundary values
    ExtractValuesFromRange( stv, stvv1, stvv2 );

    // generate OFTime objects from strings
    cond = DcmTime::getOFTimeFromString( OFString( dtv ), datasetTimeVal );
    if( cond.good() )
    {
      cond = DcmTime::getOFTimeFromString( (( stvv1 != NULL ) ? OFString( stvv1 ) : OFString("000000")), searchMaskTimeVal1 );
      if( cond.good() )
      {
        cond = DcmTime::getOFTimeFromString( (( stvv2 != NULL ) ? OFString( stvv2 ) : OFString("235959")), searchMaskTimeVal2 );
        if( cond.good() )
        {
          // now that we have the time objects we can actually
          // compare the time values and determine if it's a match

          // check if the lower value in the search mask is earlier
          // than or equal to the lower value in the dataset
          if( searchMaskTimeVal1 <= datasetTimeVal )
          {
            // now check if the upper value in the search mask is later
            // than or equal to the upper value in the dataset
            if( searchMaskTimeVal2 >= datasetTimeVal )
            {
              // if these conditions are met, then this is considered to be a match
              isMatch = OFTrue;
            }
          }
        }
      }
    }

    // free memory
    delete dtv;
    delete stv;
    if( stvv1 != NULL ) delete stvv1;
    if( stvv2 != NULL ) delete stvv2;

    // return result
    return( isMatch );
  }
  else
  {
    // so dataset time value is not given; in this case, since the time
    // value in the search mask is non-universal, we do not have a match
    return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DateTimeSingleValueMatch( const char *datasetDateValue, const char *datasetTimeValue, const char *searchMaskDateValue, const char *searchMaskTimeValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a date time single value match and returns OFTrue if the dataset's
//                and the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetDateValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                datasetTimeValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskDateValue - [in] Value for the corresponding attribute in the search mask; never NULL.
//                searchMaskTimeValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  OFDate datasetDateVal, searchMaskDateVal;
  OFTime datasetTimeVal, searchMaskTimeVal;
  OFCondition cond;

  // if we are dealing with universal matching in date and time, there is a match
  if( strcmp( searchMaskDateValue, "" ) == 0 && strcmp( searchMaskTimeValue, "" ) == 0 )
  {
    return( OFTrue );
  }
  else
  {
    // so we are not dealing with universal matching in date and time, we have to check both values individually
    OFBool dateValuesMatch = OFFalse;
    OFBool timeValuesMatch = OFFalse;

    // check date values
    if( strcmp( searchMaskDateValue, "" ) == 0 )
    {
      // a universal date always matches
      dateValuesMatch = OFTrue;
    }
    else
    {
      // so we do not have a universal date
      if( datasetDateValue == NULL )
      {
        // if there is no date value in the dataset, the non-universal date is not matched
        dateValuesMatch = OFFalse;
      }
      else
      {
        // in this case that we are dealing with a non-universal date and a
        // date value in the dataset, perform the date match

        // copy values
        char *ddv = new char[ strlen( datasetDateValue ) + 1 ];     strcpy( ddv, datasetDateValue );
        char *sdv = new char[ strlen( searchMaskDateValue ) + 1 ];  strcpy( sdv, searchMaskDateValue );

        // strip trailing spaces
        ddv = DU_stripTrailingSpaces( ddv );
        sdv = DU_stripTrailingSpaces( sdv );

        // generate OFDate objects from strings
        cond = DcmDate::getOFDateFromString( OFString( ddv ), datasetDateVal );
        if( cond.good() )
        {
          cond = DcmDate::getOFDateFromString( OFString( sdv ), searchMaskDateVal );
          if( cond.good() )
          {
            // now that we have the date objects we can actually
            // compare the date values and determine if it's a match

            // check if the date value in the search mask equals the date value in the dataset
            if( searchMaskDateVal == datasetDateVal )
            {
              // if these conditions are met, then this is considered to be a match
              dateValuesMatch = OFTrue;
            }
          }
        }

        // free memory
        delete ddv;
        delete sdv;
      }
    }

    // check time values
    if( strcmp( searchMaskTimeValue, "" ) == 0 )
    {
      // a universal time always matches
      timeValuesMatch = OFTrue;
    }
    else
    {
      // so we do not have a universal time
      if( datasetTimeValue == NULL )
      {
        // if there is no time value in the dataset, the non-universal time is not matched
        timeValuesMatch = OFFalse;
      }
      else
      {
        // in this case that we are dealing with a non-universal time and a
        // time value in the dataset, perform the time match

        // copy values
        char *dtv = new char[ strlen( datasetTimeValue ) + 1 ];     strcpy( dtv, datasetTimeValue );
        char *stv = new char[ strlen( searchMaskTimeValue ) + 1 ];  strcpy( stv, searchMaskTimeValue );

        // strip trailing spaces
        dtv = DU_stripTrailingSpaces( dtv );
        stv = DU_stripTrailingSpaces( stv );

        // generate OFTime objects from strings
        cond = DcmTime::getOFTimeFromString( OFString( dtv ), datasetTimeVal );
        if( cond.good() )
        {
          cond = DcmTime::getOFTimeFromString( OFString( stv ), searchMaskTimeVal );
          if( cond.good() )
          {
            // now that we have the time objects we can actually
            // compare the time values and determine if it's a match

            // check if the time value in the search mask equals the time value in the dataset
            if( searchMaskTimeVal == datasetTimeVal )
            {
              // if these conditions are met, then this is considered to be a match
              timeValuesMatch = OFTrue;
            }
          }
        }

        // free memory
        delete dtv;
        delete stv;
      }
    }

    // in case date and time values match, we have a match
    if( dateValuesMatch && timeValuesMatch )
      return( OFTrue );
    else
      return( OFFalse );
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::DateSingleValueMatch( const char *datasetDateValue, const char *searchMaskDateValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a date single value match and returns OFTrue if the dataset's
//                and the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetDateValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskDateValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  OFDate datasetDateVal, searchMaskDateVal;
  OFCondition cond;

  // if we are dealing with universal matching, there is a match
  if( strcmp( searchMaskDateValue, "" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching and there is a
    // date value in the dataset, compare the two date values
    if( datasetDateValue != NULL )
    {
      // initialize result
      OFBool isMatch = OFFalse;

      // copy values
      char *ddv = new char[ strlen( datasetDateValue ) + 1 ];     strcpy( ddv, datasetDateValue );
      char *sdv = new char[ strlen( searchMaskDateValue ) + 1 ];  strcpy( sdv, searchMaskDateValue );

      // strip trailing spaces
      ddv = DU_stripTrailingSpaces( ddv );
      sdv = DU_stripTrailingSpaces( sdv );

      // generate OFDate objects from strings
      cond = DcmDate::getOFDateFromString( OFString( ddv ), datasetDateVal );
      if( cond.good() )
      {
        cond = DcmDate::getOFDateFromString( OFString( sdv ), searchMaskDateVal );
        if( cond.good() )
        {
          // now that we have the date objects we can actually
          // compare the date values and determine if it's a match

          // check if the date value in the search mask equals the date value in the dataset
          if( searchMaskDateVal == datasetDateVal )
          {
            // if this condition is met, then this is considered to be a match
            isMatch = OFTrue;
          }
        }
      }

      // free memory
      delete ddv;
      delete sdv;

      // return result
      return( isMatch );
    }
    else
    {
      // if we are not dealing with universal matching and there
      // is no date value in the dataset, there is no match
      return( OFFalse );
    }
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::TimeSingleValueMatch( const char *datasetTimeValue, const char *searchMaskTimeValue )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function performs a time single value match and returns OFTrue if the dataset's
//                and the search mask's values in the corresponding attributes match; otherwise OFFalse
//                will be returned
// Parameters   : datasetTimeValue    - [in] Value for the corresponding attribute in the dataset; might be NULL.
//                searchMaskTimeValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  OFTime datasetTimeVal, searchMaskTimeVal;
  OFCondition cond;

  // if we are dealing with universal matching, there is a match
  if( strcmp( searchMaskTimeValue, "" ) == 0 )
    return( OFTrue );
  else
  {
    // if we are not dealing with universal matching and there is a
    // time value in the dataset, compare the two date values
    if( datasetTimeValue != NULL )
    {
      // initialize result
      OFBool isMatch = OFFalse;

      // copy values
      char *dtv = new char[ strlen( datasetTimeValue ) + 1 ];     strcpy( dtv, datasetTimeValue );
      char *stv = new char[ strlen( searchMaskTimeValue ) + 1 ];  strcpy( stv, searchMaskTimeValue );

      // strip trailing spaces
      dtv = DU_stripTrailingSpaces( dtv );
      stv = DU_stripTrailingSpaces( stv );

      // generate OFTime objects from strings
      cond = DcmTime::getOFTimeFromString( OFString( dtv ), datasetTimeVal );
      if( cond.good() )
      {
        cond = DcmTime::getOFTimeFromString( OFString( stv ), searchMaskTimeVal );
        if( cond.good() )
        {
          // now that we have the time objects we can actually
          // compare the time values and determine if it's a match

          // check if the time value in the search mask equals the time value in the dataset
          if( searchMaskTimeVal == datasetTimeVal )
          {
            // if this condition is met, then this is considered to be a match
            isMatch = OFTrue;
          }
        }
      }

      // free memory
      delete dtv;
      delete stv;

      // return result
      return( isMatch );
    }
    else
    {
      // if we are not dealing with universal matching and there
      // is no time value in the dataset, there is no match
      return( OFFalse );
    }
  }
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::CaseSensitiveSingleValueMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values
//                match while performing a case sensitive single value match; otherwise OFFalse
//                will be returned
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; never NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // initialize result variable
  OFBool matchFound = OFTrue;

  // copy values
  char *dv = new char[ strlen( datasetValue ) + 1 ];
  strcpy( dv, datasetValue );
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // strip trailing spaces
  dv = DU_stripTrailingSpaces( dv );
  sv = DU_stripTrailingSpaces( sv );

  // perform match
  if( strcmp( dv, sv ) != 0 )
    matchFound = OFFalse;

  // free memory
  delete dv;
  delete sv;

  // return result
  return( matchFound );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::WildcardMatch( const char *datasetValue, const char *searchMaskValue )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function returns OFTrue if the dataset's and the search mask's values
//                match while performing a wildcard match; otherwise OFFalse will be returned.
// Parameters   : datasetValue    - [in] Value for the corresponding attribute in the dataset; never NULL.
//                searchMaskValue - [in] Value for the corresponding attribute in the search mask; never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // initialize return value
  OFBool ok = OFFalse;

  // copy values
  char *dv = new char[ strlen( datasetValue ) + 1 ];
  strcpy( dv, datasetValue );
  char *sv = new char[ strlen( searchMaskValue ) + 1 ];
  strcpy( sv, searchMaskValue );

  // remember the pointers for delete
  char *dvanchor = dv;
  char *svanchor = sv;

  // strip trailing spaces
  dv = DU_stripTrailingSpaces( dv );
  sv = DU_stripTrailingSpaces( sv );

  // go through both strings character by character as long as
  // a) we do not see an EOS in sv AND
  // b) we do not see a '*' in sv AND
  // c) the current character in dv is equal to the
  //    current character in the sv, or sv contains
  //    a '?' and sv does not yet show an EOS.
  while( *sv != '\0' &&
         *sv != '*'  &&
         ( *sv == *dv || ( *sv == '?' && *dv != '\0' ) ) )
  {
    sv++;
    dv++;
  }

  // if the current pattern character equals the star symbol (wild card symbol) we need
  // to call another function. If this is not the case, ok will be set to OFTrue or
  // OFFalse, depending on if we got to the end of both strings in the above loop.
  if( *sv == '*' )
    ok = MatchStarSymbol( dv, sv+1 );
  else
  {
    if( *sv == '\0' && *dv == '\0' )
      ok = OFTrue;
    else
      ok = OFFalse;
  }

  // free memory
  delete dvanchor;
  delete svanchor;

  // return result
  return( ok );
}

// ----------------------------------------------------------------------------

OFBool WlmDBInteractionManager::MatchStarSymbol( const char *dv, const char *sv )
// Date         : July 12, 2002
// Author       : Thomas Wilkens
// Task         : This function is called, if the search pattern contains a star symbol. It determines
//                if dv (the dataset's value) still matches sv (the search mask's value). This function
//                takes the star symbol in sv into account. (Note that the pattern value might contain
//                more wild card symbols.) The function will return OFTrue if there is a match; if there
//                is not a match it will return OFFalse.
// Parameters   : dv - [in] Dataset's value; never NULL.
//                sv - [in] Search mask's value (may contain wild card symbols); never NULL.
// Return Value : OFTrue if the values match, OFFalse otherwise.
{
  // initialize result value
  OFBool ok = OFFalse;

  // move pointer one char to the right as long as it points to a star symbol
  while( *sv == '*' )
    sv++;

  // if we got to the end of sv, return OFTrue
  if( *sv == '\0' )
    return OFTrue;

  // if there is something else at the end of sv,
  // we need to go ahead and compare the rest of the two strings

  // as long as ok equals OFFalse and we did not get to the end of the string
  while( !ok && *dv != '\0' )
  {
    // if sv reveals a '?' or if both pointers refer to the same
    // character, we need to call WildcardMatch again, to determine a result
    if( *sv == '?' || *dv == *sv )
      ok = WildcardMatch( dv+1, sv+1 );

    // if ok still equals OFFalse, set pointer one character to the right
    if( !ok )
      dv++;
  }

  // return result
  return( ok );
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::ExtractValuesFromRange( const char *range, char *&lower, char *&upper )
// Date         : July 15, 2002
// Author       : Thomas Wilkens
// Task         : This function extracts the actual lower and upper date or time values from a given
//                date or time range.
// Parameters   : range - [in] Date or time range from which lower and upper values shall be extracted.
//                lower - [out] Newly created string specifying the lower value from the date/time range;
//                        NULL if value is not specified in range.
//                upper - [out] Newly created string specifying the upper value from the date/time range;
//                        NULL if value is not specified in range.
// Return Value : none.
{
  // get lower value
  const char *tmp = strchr( range, '-' );
  int res = tmp - range;
  if( res == 0 )
    lower = NULL;
  else
  {
    lower = new char[ res + 1 ];
    strncpy( lower, range, res );
    lower[res] = '\0';
  }

  // get upper value
  int len = strlen( range );
  if( res == len - 1 )
    upper = NULL;
  else
  {
    upper = new char[ len - 1 - res + 1 ];
    strncpy( upper, tmp + 1, len - 1 - res );
    upper[len-1-res] = '\0';
  }
}

// ----------------------------------------------------------------------------

void WlmDBInteractionManager::StartGenerate(DcmDataset *searchMask)
{
  queryContext = 0;
}

void WlmDBInteractionManager::StopGenerate()
{
  queryContext = 0;
}

DcmDataset *WlmDBInteractionManager::NextDataset(DcmDataset *searchMask)
{
  char buf[65];
  int i = 0;
  DcmDataset *dataset = NULL;

  if(queryContext == 3)
  {
	queryContext = 0;
	return NULL;
  }
  else
  {
	dataset = new DcmDataset();

	if(queryContext == 1)
	{
	  DcmCodeString *charset = new DcmCodeString(DcmTag(DCM_SpecificCharacterSet));	//0008,0005 O 1
	  charset->putString("ISO_IR 100");
	  dataset->insert(charset);
	}
	else if(queryContext == 2)
	{
	  DcmCodeString *charset = new DcmCodeString(DcmTag(DCM_SpecificCharacterSet));	//0008,0005 O 1
	  charset->putString("GB18030");
	  dataset->insert(charset);
	}
	i = queryContext++;
  }

  DcmShortString *accessNum = new DcmShortString(DcmTag(DCM_AccessionNumber));  //0008,0050 O 2
  sprintf(buf, "ACC-%06d", i);
  accessNum->putString(buf);
  dataset->insert(accessNum);

  DcmPersonName *referPhyName = new DcmPersonName(DcmTag(DCM_ReferringPhysiciansName));  // 0008,0090 O 2
  referPhyName->putString("Physician^Referring=医师^转诊");
  dataset->insert(referPhyName);

  DcmSequenceOfItems *referStudySQ = new DcmSequenceOfItems(DCM_ReferencedStudySequence); //0008,1110 O 2
  dataset->insert(referStudySQ);

  DcmSequenceOfItems *referPatientSQ = new DcmSequenceOfItems(DCM_ReferencedPatientSequence); //0008,1120 O 2
  dataset->insert(referPatientSQ);

  DcmPersonName *patientName = new DcmPersonName(DcmTag(DCM_PatientsName));  // 0010,0010 R 1
  sprintf(buf, "Test^%d=测试^%d", i, i);
  patientName->putString(buf);
  dataset->insert(patientName);

  DcmLongString *patientId = new DcmLongString(DcmTag(DCM_PatientID));  //0010,0020 R 1
  sprintf(buf, "PATID-%06d", i, i);
  patientId->putString(buf);
  dataset->insert(patientId);

  DcmDate *birthDate = new DcmDate(DcmTag(DCM_PatientsBirthDate));	//0010,0030 O 2
  birthDate->setCurrentDate();
  dataset->insert(birthDate);

  DcmCodeString *patientSex = new DcmCodeString(DcmTag(DCM_PatientsSex));	//0010,0040 O 2
  patientSex->putString("M");
  dataset->insert(patientSex);

  DcmAgeString *patientAge = new DcmAgeString(DcmTag(DCM_PatientsAge)); //0010,1010 O 3
  patientAge->putString("50Y");
  dataset->insert(patientAge);

  DcmDecimalString *patientWeight = new DcmDecimalString(DcmTag(DCM_PatientsWeight));  //0010,1030 O 2
  patientWeight->putString("70");
  dataset->insert(patientWeight);

  DcmUniqueIdentifier *studyInstUid = new DcmUniqueIdentifier(DcmTag(DCM_StudyInstanceUID));  //0020,000d O 1
  sprintf(buf, "1.2.156.126666.1.2.1.%d", i);
  studyInstUid->putString(buf);
  dataset->insert(studyInstUid);

  DcmPersonName *reqPhyName = new DcmPersonName(DcmTag(DCM_RequestingPhysician));  // 0032,1032 O 2
  reqPhyName->putString("Physician^Requesting=医师^请求");
  dataset->insert(reqPhyName);

  DcmLongString *reqProcDesc = new DcmLongString(DcmTag(DCM_RequestedProcedureDescription));  //0032,1060 O 1
  reqProcDesc->putString("Requested Procedure Test");
  dataset->insert(reqProcDesc);

  DcmLongString *admissionId = new DcmLongString(DcmTag(DCM_AdmissionID));  //0038,0010 O 2
  sprintf(buf, "ADMID-%06d", i, i);
  admissionId->putString(buf);
  dataset->insert(admissionId);

  DcmShortString *reqProcPrior = new DcmShortString(DcmTag(DCM_RequestedProcedurePriority));  //0040,1003 O 2
  reqProcPrior->putString("MEDIUM"); //STAT, HIGH, ROUTINE, MEDIUM, LOW 
  dataset->insert(reqProcPrior);

  DcmSequenceOfItems *schdlProcSQ = new DcmSequenceOfItems(DCM_ScheduledProcedureStepSequence); //0040,0100 R 1
  {
	DcmItem *item = new DcmItem();

	DcmCodeString *modality = new DcmCodeString(DcmTag(DCM_Modality));	//0008,0060 R 1
	modality->putString("MR");
	item->insert(modality);

	DcmApplicationEntity *schdlStationAE = new DcmApplicationEntity(DCM_ScheduledStationAETitle);  //0040,0001 R 1
	schdlStationAE->putString("regstation");
	item->insert(schdlStationAE);

	DcmDate *schdlDate = new DcmDate(DcmTag(DCM_ScheduledProcedureStepStartDate));	//0040,0002 R 1
	schdlDate->setCurrentDate();
	item->insert(schdlDate);

	DcmTime *schdlTime = new DcmTime(DCM_ScheduledProcedureStepStartTime);  //0040,0003 R 1
	schdlTime->setCurrentTime();
	item->insert(schdlTime);

	DcmPersonName *schdlPerfmPhyName = new DcmPersonName(DcmTag(DCM_ScheduledPerformingPhysiciansName));  // 0040,0006 R 2
	schdlPerfmPhyName->putString("Physician^SchedulePerform=医师^预约执行");
	item->insert(schdlPerfmPhyName);

	DcmLongString *schdlProcStepDesc = new DcmLongString(DcmTag(DCM_ScheduledProcedureStepDescription));  //0040,0007 O 1
	schdlProcStepDesc->putString("Scheduled Procedure Step Test");
	item->insert(schdlProcStepDesc);

	DcmShortString *schdlProcStepId = new DcmShortString(DcmTag(DCM_ScheduledProcedureStepID));  //0040,0009 O 1
	sprintf(buf, "SCHD-%06d", i);
	schdlProcStepId->putString(buf);
	item->insert(schdlProcStepId);

	schdlProcSQ->append(item);
  }
  dataset->insert(schdlProcSQ);

  DcmShortString *reqProcStepId = new DcmShortString(DcmTag(DCM_RequestedProcedureID));  //0040,1001 O 1
  sprintf(buf, "REQ-%06d", i);
  reqProcStepId->putString(buf);
  dataset->insert(reqProcStepId);

  return dataset;
}
