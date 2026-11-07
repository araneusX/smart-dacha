#pragma once

void wifiBegin();
void wifiLoop();
bool wifiIsConnected();
bool wifiWasDisconnected();
void wifiClearDisconnectFlag();
bool timeIsSynced();
int historySlotId();
bool historyMinute();
