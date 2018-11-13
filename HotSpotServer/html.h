#ifndef __HTML_H__
#define __HTML_H__

#include <WiFi.h>

void pageTitle(WiFiClient client, String title) {
    client.println(R"(
        <div class="col">
        <span class="lead align-right" style="font-size:1.5em;">)" + title + R"(</span>
    </div>
    </div>
    </div>
    )");
}

void startForm(WiFiClient client, String action) {
    client.println(R"(<form action=")" + action + R"(" method="GET">)");
}

void endForm(WiFiClient client) {
    client.println("</form>");
}

void endDiv(WiFiClient client) {
    client.println("</div>");
}
void inputBox(WiFiClient client, String name, String nameVar, String label, bool smallLine, String smallLineText) {
    client.println(R"(<div class="form-group">)");
    client.println(R"(    <label for=")" + name + R"(">Top Line</label>)");
    client.println(R"(    <input type="text" class="form-control" name=")" + name + R"(" id=")" + name + R"(" value=")" + nameVar + R"(">)");
    if(smallLine){
        client.println(R"(<small id=")" + name + R"(Help" class="form-text text-muted">)" + smallLineText + R"(</small>)");
    }
    client.println(R"(<div>)");
}
void checkBox(WiFiClient client, String name, String nameVar, String label) {
    client.println(R"(<div>
        <input type="checkbox" id=")" + name + R"(" name=")" + name + R"(" value=")" + name + R"(" ")" + nameVar + R"(">
    <label for=")" + name + R"(">)" + label + R"(</label></div>)");
}

#endif
