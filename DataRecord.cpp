/*
 * \brief Data record and data storage (flash)
 */

#include <Arduino.h>
#include <Sodaq_dataflash.h>

//#include "Diag.h"
#include "DataRecord.h"

myRecord rec;

/*
 * \brief Is the record a valid record
 */
bool isValidRecord(myRecord *rec)
{
  if ((int32_t)rec->ts == -1) {
    return false;
  }
  return true;
}

/*
 * \brief Read a whole page into a buffer
 *
 * Buffer_Read_Str returns corrupted data if we're trying
 * to read the whole page at once. Here we do it in chunks
 * of 16 bytes.
 */
void readPage(int page, uint8_t *buffer, unsigned int size)
{
  dflash.readPageToBuf1(page);
  // Read in chunks of max 16 bytes
  int byte_offset = 0;
  while (size > 0) {
    int size1 = size >= 16 ? 16 : size;
    dflash.readStrBuf1(byte_offset, buffer, size1);
    byte_offset += size1;
    buffer += size1;
    size -= size1;
  }
}

/*
 * \brief Read the page header
 */
bool readPageHeader(int page, myHeader *hdr)
{
  size_t byte_offset = 0;
  size_t size = sizeof(myHeader);

  dflash.readPageToBuf1(page);
  uint8_t *buffer = (uint8_t *)hdr;
  while (size > 0) {
    int size1 = size >= 16 ? 16 : size;
    dflash.readStrBuf1(byte_offset, buffer, size1);
    byte_offset += size1;
    buffer += size1;
    size -= size1;
  }
  return isValidHeader(hdr);
}

/*
 * \brief Read one record from the page
 */
bool readPageNthRecord(int page, int nth, myRecord *rec)
{
  size_t byte_offset = sizeof(myHeader) + nth * sizeof(myRecord);
  size_t size = sizeof(myRecord);
  if ((byte_offset + size) > DF_PAGE_SIZE) {
    // The record is crossing page boundary
    rec->ts = -1;
    return false;
  }

  dflash.readPageToBuf1(page);
  uint8_t *buffer = (uint8_t *)rec;
  while (size > 0) {
    int size1 = size >= 16 ? 16 : size;
    dflash.readStrBuf1(byte_offset, buffer, size1);
    byte_offset += size1;
    buffer += size1;
    size -= size1;
  }
  return isValidRecord(rec);
}

/*
 * \brief Is this a valid page header
 */
bool isValidHeader(myHeader *hdr)
{
  if (strncmp(hdr->magic, HEADER_MAGIC, sizeof(HEADER_MAGIC)) != 0) {
    return false;
  }
  // The timestamp should be OK too. How can it be bad?
  return true;
}

/*
 * \brief Is this a valid page to upload
 *
 * Note. This invalidates the "internal buffer" of the Data Flash
 */
bool isValidUploadPage(int page)
{
  myHeader hdr;
  readPageHeader(page, &hdr);

  return isValidHeader(&hdr);
}

/*
 * \brief Search for curPage and uploadPage in the data flash
 *
 * Search through the whole date flash and try to find the best page for
 * curPage and for the uploadPage.
 */
void findCurAndUploadPage(int *curPage, int *uploadPage, uint16_t randomNum)
{
#if ENABLE_DIAG
  uint32_t start = millis();
#endif

  int myCurPage = -1;
  int myUploadPage = -1;
  uint32_t uploadTs;
  myHeader hdr;

  // First round, search for upload page
  for (int page = 0; page < DF_NR_PAGES; ++page) {
    readPage(page, (uint8_t*)&hdr, sizeof(hdr));

    if (isValidHeader(&hdr)) {
      if (myUploadPage < 0) {
        myUploadPage = page;
        uploadTs = hdr.ts;
      } else {
        // Make sure we remember the oldest upload record
        if (hdr.ts < uploadTs) {
          myUploadPage = page;
          uploadTs = hdr.ts;
        }
      }
    }
  }

  if (myUploadPage >= 0) {
    // Starting from upload page, look for the next free spot.
    // TODO Verify this logic
    int page = myUploadPage;
    for (int nr = 0; nr < DF_NR_PAGES; ++nr, page = getNextPage(page)) {
      readPage(page, (uint8_t*)&hdr, sizeof(hdr));
      if (!isValidHeader(&hdr)) {
        myCurPage = page;
        break;
      }
    }
    if (myCurPage < 0) {
      // None of the pages is empty.
      // Use the page of the oldest upload, the caller will take care of this
      myCurPage = myUploadPage;
    }
  } else {
    // No upload page found.
    // Start at a random place
    myCurPage = randomNum % DF_NR_PAGES;
    myUploadPage = -1;
  }

  *curPage = myCurPage;
  *uploadPage = myUploadPage;

#if ENABLE_DIAG
  uint32_t elapse = millis() - start;
  DIAGPRINT(F("Find uploadPage in (ms) ")); DIAGPRINTLN(elapse);
#endif
}

uint32_t getPageTS(int page)
{
  if (page < 0) {
    return -1;
  }
  myHeader hdr;
  readPage(page, (uint8_t*)&hdr, sizeof(hdr));
  return hdr.ts;
}

//################ print all values of the record ################
#ifdef ENABLE_DIAG
void printRecord(myRecord * rec)
{
  DIAGPRINT(F("EPOCH: "));
  DIAGPRINTLN(rec->ts);

  DIAGPRINT(F("Rain: "));
  DIAGPRINTLN(rec->rain_ticks);

  DIAGPRINT(F("Windticks: "));
  DIAGPRINT(rec->wind_ticks);
  DIAGPRINT(F(", Winddir: "));
  DIAGPRINTLN(rec->wind_dir);

  DIAGPRINT(F("Windticks Gust: "));
  DIAGPRINT(rec->wind_gust_ticks);
  DIAGPRINT(F(", Winddir: "));
  DIAGPRINTLN(rec->wind_gust_dir);

  DIAGPRINT(F("Windticks Lull: "));
  DIAGPRINT(rec->wind_lull_ticks);
  DIAGPRINT(F(", Winddir: "));
  DIAGPRINTLN(rec->wind_lull_dir);

  DIAGPRINT(F("Battery Volt: "));
  DIAGPRINTLN(rec->batteryVoltage);
}

/*
 * \brief This function reads all pages in the data flash and measures how long
 * it takes.
 *
 * This function is just meant for diagnostics.
 */
void readAllPages()
{
#if 0
  uint32_t start = millis();
#endif

  for (uint16_t page = 0; page < DF_NR_PAGES; page++) {
    myHeader hdr;
    readPage(page, (uint8_t*)&hdr, sizeof(hdr));
  }

#if 0
  uint32_t elapse = millis() - start;
  DIAGPRINT(F("Nr secs to read all pages: ")); DIAGPRINTLN(elapse / 1000);
#endif
}

/*
 * \brief Dump the contents of a data flash page
 */
void dumpPage(int page)
{
  if (page < 0)
    return;

  DIAGPRINT(F("page ")); DIAGPRINTLN(page);
  dflash.readPageToBuf1(page);
  uint8_t buffer[16];
  for (uint16_t i = 0; i < DF_PAGE_SIZE; i += sizeof(buffer)) {
    size_t nr = sizeof(buffer);
    if ((i + nr) > DF_PAGE_SIZE) {
      nr = DF_PAGE_SIZE - i;
    }
    dflash.readStrBuf1(i, buffer, nr);

    dumpBuffer(buffer, nr);
  }
}

#endif
