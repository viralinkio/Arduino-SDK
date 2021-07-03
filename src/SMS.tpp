#ifndef VIRALINK_SMS_TPP
#define VIRALINK_SMS_TPP

#include <locale>
#include <codecvt>

class SMS {
public:
    SMS(String rawData);
    SMS();

    uint8_t index;
    String time;
    String phone;
    String text;

    static void stringToUTF16(String txt, String result[]);

    static String utf16ToString(String txt);

    static String utf8ToString(String txt);
};

void SMS::stringToUTF16(String txt, String *result) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    auto u16 = convert.from_bytes(txt.c_str());
    uint16_t size = txt.length() / 2;

    for (int i = 0; i < size; i++) {
        String p1 = String(lowByte(u16[i]), HEX);
        String p2 = String(highByte(u16[i]), HEX);
        result[i] = (p1.length() == 1 ? "0" + p1 : p1) + (p2.length() == 1 ? "0" + p2 : p2);
    }
}

String SMS::utf16ToString(String txt) {
    if (txt.length() % 4 != 0) {
        return "Can Not Read";
    }
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    int size = txt.length() / 4;
    int utf8Size=0;
    wchar_t utf16[size];
    for (int i = 0; i < size; i++) {
        if (!txt.substring(i * 4, i * 4 + 2).equals("00")) utf8Size++;
        if (!txt.substring(i * 4 + 2, i * 4 + 4).equals("00")) utf8Size++;
        utf16[i] = strtol(txt.substring(i * 4, i * 4 + 4).c_str(), nullptr, 16);
    }
    auto utf8 = convert.to_bytes(utf16);
    String result = "";
    String utf8S = "";
    for (int i = 0; i < utf8Size; i++) {
//        char h[2];
//        sprintf(h,"%02X",utf8[i]);
//        utf8S.concat(h);
        result.concat((char) utf8[i]);
    }

//    Serial.println("*");
//    Serial.println(utf8S);
//    Serial.println("*");
//    Serial.println(result);
    return result;
}

String SMS::utf8ToString(String txt) {
    if (txt.length() % 2 != 0) {
        return "Can Not Read";
    }
    int size = txt.length() / 2;
    uint8_t bf[size];
    for (int i = 0; i < size; i++)
        bf[i] = strtol(txt.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    String result = "";
    for (int i = 0; i < size; i++)
        result.concat((char) bf[i]);
    return result;
}

SMS::SMS(String rawData) {
    int startIndex = rawData.indexOf("+CMGL");
    int endIndex = rawData.indexOf("\n", startIndex);

    String info = rawData.substring(startIndex, endIndex);
    String payload = rawData.substring(endIndex + 1);
    payload.trim();

    startIndex = info.indexOf(":");
    endIndex = info.indexOf(",");
    this->index = info.substring(startIndex + 1, endIndex).toInt();

    startIndex = info.indexOf(",", endIndex + 1);
    endIndex = info.indexOf(",", startIndex + 1);
    this->phone = utf8ToString(info.substring(startIndex + 2, endIndex - 1));
    this->phone.trim();

    startIndex = info.indexOf(",", endIndex + 1);
    endIndex = info.indexOf(",", startIndex + 1);
    endIndex = info.indexOf(",", endIndex + 1);
    this->time = info.substring(startIndex + 2, endIndex - 1);
    this->time.trim();

    startIndex = info.indexOf(",", endIndex + 1);
    startIndex = info.indexOf(",", startIndex + 1);
    startIndex = info.indexOf(",", startIndex + 1);
    endIndex = info.indexOf(",", startIndex + 1);

    if (info.substring(startIndex + 1, endIndex).toInt() == 8)
        text = utf16ToString(payload);
    else if (info.substring(startIndex + 1, endIndex).toInt() == 0)
        text = utf8ToString(payload);
    else text = "Not Supported Text Format";

}

SMS::SMS() {

}

#endif //VIRALINK_SMS_TPP