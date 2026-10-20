#ifndef HARNESS_BLOCK_SCAN_H
#define HARNESS_BLOCK_SCAN_H

void BlockScanEnable();

void BlockScanTick(unsigned tick);

void BlockScanWrite(const char* commit);

#endif
