#ifndef HARNESS_BLOCK_SCAN_H
#define HARNESS_BLOCK_SCAN_H

void BlockScanEnable(unsigned every);

void BlockScanTick(unsigned tick, unsigned endTick);

void BlockScanWrite(const char* commit);

#endif
