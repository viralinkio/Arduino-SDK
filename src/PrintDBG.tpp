#ifndef VIRALINK_PRINTDBG_TPP
#define VIRALINK_PRINTDBG_TPP

void printDBGln(const char *text) {
#ifdef VIRALINK_DEBUG
    SerialMon.println(text);
#endif
}

void printDBG(const char *text) {
#ifdef VIRALINK_DEBUG
    SerialMon.print(text);
#endif
}

#endif //VIRALINK_PRINTDBG_TPP
