#ifndef DATARECORD_H
#define DATARECORD_H

#include <Sodaq_dataflash.h>

#define HEADER_MAGIC            "wxftp"
#define DATA_VERSION            3               // Please register at http://sodaq.net/
struct myHeader
{
  uint32_t      ts;			// timestamp
  uint16_t      page;		// flash page number
  uint32_t      version;	// data version
  char          magic[6];	// magic string
};
typedef struct myHeader myHeader;

struct myRecord
{
  // seconds since epoch (01-jan-1970)
  uint32_t      ts;             

  // flash page number
  uint16_t      page;

  // raincounter
  uint16_t      rain_ticks;

  // wind
  uint16_t      wind_ticks;
  uint16_t      wind_gust_ticks;
  uint16_t      wind_lull_ticks;
  uint16_t      wind_dir;
  uint16_t      wind_gust_dir;
  uint16_t      wind_lull_dir;

  // battery
  uint16_t      batteryVoltage;
  uint16_t      temperatureRTC;
  
  // humidity
  uint16_t      humidity;
  uint16_t      temperatureH;
  
  // pressure
  uint16_t      pressure;
  uint16_t      temperatureP;
};
typedef struct myRecord myRecord;
//extern myRecord rec;

#define NR_RECORDS_PER_PAGE     ((DF_PAGE_SIZE - sizeof(myHeader)) / sizeof(myRecord))

static inline int getNextPage(int page)
{
  page++;
  if (page >= DF_NR_PAGES) {
    page = 0;
  }
  return page;
}

bool isValidRecord(myRecord *rec);
bool isValidHeader(myHeader *hdr);
bool isValidUploadPage(int page);
void findCurAndUploadPage(int *curPage, int *uploadPage, uint16_t randomNum);
uint32_t getPageTS(int page);
void readPage(int page, uint8_t *buffer, unsigned int size);
bool readPageNthRecord(int page, int nth, myRecord *rec);
bool readPageHeader(int page, myHeader *hdr);

#ifdef ENABLE_DIAG
void printRecord(myRecord *rec);
void readAllPages();
void dumpPage(int page);
#else
#define printRecord(rec)
#define readAllPages()
#define dumpPage(page)
#endif

#endif // DATARECORD_H
