#ifndef __HTML_H__
#define __HTML_H__

#include <WiFi.h>
void htmlHead(WiFiClient client) {
  // HTML head

  // put these in head if we want images.  but need real images

            //<link rel="apple-touch-icon" sizes="72x72" href="/">
            //<link rel="apple-touch-icon" sizes="114x114" href="/">
            //<link rel="apple-touch-icon" href="/">
            //<link rel="apple-touch-startup-image" href="/">
  client.println(R"(
      <!DOCTYPE html>
      <html>
      <head>
          <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
          <meta name="apple-mobile-web-app-capable" content="yes">
          <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">

          <meta http-equiv="X-UA-Compatible" content="IE=edge">
          <meta name="HandheldFriendly" content="true">
          <meta name="MobileOptimized" content="width">

          <title>Pro Tournament Scales Hotspot Printer</title>
          <style>.middle-form{max-width: 500px; margin:auto;padding:10px;}
      )");
      insertCSS(client);
  client.println("</head>");
  client.println("<body>");
}

void printPTSLogo(WiFiClient client) {
  client.println(R"(
      <div class="middle-form" style="margin-bottom: 10px;">
      <div class="container">
      <div class="row">
      <div class="col">

      <div style="width: 125px;""><a href="/">
          <svg id="Layer_1" data-name="Layer 1" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1577.22 1100.85">
            <title>Pro Tournament Scales</title>
            <path d="M1581.78,259.77" transform="" style="fill: #275589"/>
            <path d="M1544.44,434.34" transform="" style="fill: #fff200"/>
            <path d="M829.65,1104.3" transform="" style="fill: #275589"/>
            <path d="M831.39,1100.59" transform="" style="fill: #275589"/>
            <path d="M1538.69,242.23a27.37,27.37,0,0,0-2.84-.06l-.06-.06H1284.7l1.84-8.91,37.91-183S1331.16,0,1283.6,0H545.79c-34.51,0-40.85,33.25-40.85,33.25L467,216.27l-5.15,24.85H174.18s-33.48-1.24-40.8,33l-23.71,111L98.9,435.56l-3,14.2-94,444s-15.32,50.43,40.8,50.43H175.24s33,3.33,40.8-33l40.37-189H440.34c38.52-3.75,60.17,6.51,71.71,14.32,78.72,53.28,46.42,235.56,32.55,313.84-.25,1.41.2,2.9.33,4.36-.08.48-.19.78-.27,1.29-6.89,46.47,40.77,44.59,40.77,44.59h212s32.79,5.4,40.84-33.19l24.84-119h387.07c33.44,0,60.78-1.75,83.58-5.36,27-4.27,50.07-13.31,68.69-26.89,19-13.87,34.13-32.68,45-55.93,9.24-19.72,17.23-44.47,24.41-75.65l22.74-103.89a236.78,236.78,0,0,0,4.2-44.48c0-60.83-33.53-97.9-33.53-97.9-41.59-46.3-107.7-39.77-114.68-39a338.1,338.1,0,0,1,30.64-18.91c48-26.18,95.3-36.76,135.11-40.71v-.12a40.66,40.66,0,0,0,35.6-30.8c.31-1.42.61-2.83.92-4.25l23.71-112.08a37.6,37.6,0,0,0,.48-9.81C1575.39,262.39,1556.82,243.51,1538.69,242.23ZM568,656.23q-19.95,14.55-50.64,19.4t-77.05,4.84H222.66l-47.42,222H42.71l94.82-444H270.09L246.37,568.39h168.1a347.87,347.87,0,0,0,37.18-1.61q14.55-1.62,23.71-8.08t14-19.4q4.83-12.91,10.22-35.56l9.7-45.27q2.18-11.82,3.77-20.47a79,79,0,0,0,1.63-14c0-12.23-4.16-20.29-12.41-24.27s-25-5.9-50.09-5.9H150.47l23.71-111H524.39q74.36,0,104,14.54T658,355q0,24.81-7.54,59.28L621.38,549q-9.72,42-21.55,67.35T568,656.23Zm229.9,400.46c-6.51.46-13.56,1.21-21.06,2.18H585.43L759.54,224.74H507.86l37.93-183H1283.6l-37.91,183H971.58ZM1512.08,395.9H1203.89a347.66,347.66,0,0,0-37.17,1.61q-14.55,1.62-23.72,8.09T1128.46,425q-5.42,12.93-9.7,35.57l-3.24,14q-5.4,26.94-5.39,35.56,0,18.35,12.93,23.71t50.64,5.38h180q103.42,0,103.44,97a192.17,192.17,0,0,1-3.24,35.57l-22.63,103.44q-9.69,42-21.55,67.34t-31.78,39.89q-20,14.55-50.64,19.39t-77.06,4.85H891.39L915.1,794.6h309.26a348.13,348.13,0,0,0,37.18-1.61q14.55-1.62,23.71-8.09c6.09-4.29,10.78-10.77,14-19.39s6.64-20.47,10.24-35.55l3.22-14q5.41-26.93,5.4-36.65,0-28-36.65-28H1091.81q-58.17,0-91.58-24.79t-33.41-66.81a90.26,90.26,0,0,1,.53-10.23c.36-3.24.89-6.64,1.63-10.25L995.91,415.3q9.72-42,22.1-67.9t32.33-39.86q19.92-14,50.64-18.87t76-4.84h358.85Z" transform="" style="fill: #275589"/>
          </svg></a>
      </div> <!--// style -->

      </div> <!--// col -->
  )");
}

void pageTitle(WiFiClient client, String title) {
    client.println(R"(
    <div class="col">
        <span class="lead align-right" style="font-size:1.5em;">)" + title + R"(</span>
    </div> <!--// col -->
    </div> <!--// row (from printPTSLogo) -->
    </div> <!--// container (from printPTSLogo) -->
    )");
    client.println("<div class=\"middle-form\">");
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
void inputBox(WiFiClient client, String name, String nameVar, String label, bool smallLine, String smallLineText, String type="text") {
    client.println(R"(<div class="form-group">)");
    client.println(R"(    <label for=")" + name + R"(">)" + label + R"(</label>)");
    client.println(R"(    <input type=")" + type + R"(" class="form-control" name=")" + name + R"(" id=")" + name + R"(" value=")" + nameVar + R"(">)");
    if(smallLine){
        client.println(R"(<small id=")" + name + R"(Help" class="form-text text-muted">)" + smallLineText + R"(</small>)");
    }
    client.println(R"(</div>)");
}
void checkBox(WiFiClient client, String name, String nameVar, String label) {
    client.println(R"(<div>
        <input type="checkbox" id=")" + name + R"(" name=")" + name + R"(" value=")" + name + R"(" )" + nameVar + R"(>
    <label for=")" + name + R"(">)" + label + R"(</label></div>)");
}
void button(WiFiClient client, String value, String context) {
  client.println(R"(<input style="margin-bottom:5px;" type="submit" value=")" + value + R"(" class="btn btn-)" + context + R"( btn-lg btn-block">)");
}

void printButton(WiFiClient client){
  client.println(R"(
      <button type="submit" value="Print" id="print" style="height:250px;margin-bottom:25px;" class="btn btn-primary btn-lg btn-block">
<div style="margin:auto 35%;color:white;fill:white;">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path d="M432 192h-16v-82.75c0-8.49-3.37-16.62-9.37-22.63L329.37 9.37c-6-6-14.14-9.37-22.63-9.37H126.48C109.64 0 96 14.33 96 32v160H80c-44.18 0-80 35.82-80 80v96c0 8.84 7.16 16 16 16h80v112c0 8.84 7.16 16 16 16h288c8.84 0 16-7.16 16-16V384h80c8.84 0 16-7.16 16-16v-96c0-44.18-35.82-80-80-80zM320 45.25L370.75 96H320V45.25zM128.12 32H288v64c0 17.67 14.33 32 32 32h64v64H128.02l.1-160zM384 480H128v-96h256v96zm96-128H32v-80c0-26.47 21.53-48 48-48h352c26.47 0 48 21.53 48 48v80zm-80-88c-13.25 0-24 10.74-24 24 0 13.25 10.75 24 24 24s24-10.75 24-24c0-13.26-10.75-24-24-24z"/></svg>
</div>
<br>Print Ticket
      </button>)");
}

void bottomNav(WiFiClient client, int major, int minor, int patch){
  client.println(R"(
      </div>
  <nav class="navbar bottom navbar-light bg-light">
      <div class="middle-form">
          <a class="navbar-brand text-right" href="#"><p class="text-right text-muted">version: )" + String(major) + R"(.)" + String(minor) + R"(.)" + String(patch) + R"(</p></a>
      </div>
  </nav>
  )");
  client.println("</div>");
  client.println("</body>");
  client.println("</html>");
}

void alert(WiFiClient client, String context, String contentTop, String heading = "", String contentBottom = ""){
  client.println("<div style=\"margin-top:5px;\" class=\"alert alert-" + context + "\" role=\"alert\">");
  if(heading != ""){
    client.println("<h4 class=\"alert-heading\">" + heading + "</h4>");
  }
  client.println(contentTop);
  if(contentBottom != ""){
    client.println("<hr>");
    client.println("<p class=\"mb-0\">" + contentBottom + "</p>");
  }
  client.println("</div>");
}

void stateBox(WiFiClient client) {
client.println(R"(
  <div class="form-group col-md-4">
    <label for="inputState">State</label>
    <select id="inputState" class="form-control">
	<option value="AL">Alabama</option>
	<option value="AK">Alaska</option>
	<option value="AZ">Arizona</option>
	<option value="AR">Arkansas</option>
	<option value="CA">California</option>
	<option value="CO">Colorado</option>
	<option value="CT">Connecticut</option>
	<option value="DE">Delaware</option>
	<option value="DC">District Of Columbia</option>
	<option value="FL">Florida</option>
	<option value="GA">Georgia</option>
	<option value="HI">Hawaii</option>
	<option value="ID">Idaho</option>
	<option value="IL">Illinois</option>
	<option value="IN">Indiana</option>
	<option value="IA">Iowa</option>
	<option value="KS">Kansas</option>
	<option value="KY">Kentucky</option>
	<option value="LA">Louisiana</option>
	<option value="ME">Maine</option>
	<option value="MD">Maryland</option>
	<option value="MA">Massachusetts</option>
	<option value="MI">Michigan</option>
	<option value="MN">Minnesota</option>
	<option value="MS">Mississippi</option>
	<option value="MO">Missouri</option>
	<option value="MT">Montana</option>
	<option value="NE">Nebraska</option>
	<option value="NV">Nevada</option>
	<option value="NH">New Hampshire</option>
	<option value="NJ">New Jersey</option>
	<option value="NM">New Mexico</option>
	<option value="NY">New York</option>
	<option value="NC">North Carolina</option>
	<option value="ND">North Dakota</option>
	<option value="OH">Ohio</option>
	<option value="OK">Oklahoma</option>
	<option value="OR">Oregon</option>
	<option value="PA">Pennsylvania</option>
	<option value="RI">Rhode Island</option>
	<option value="SC">South Carolina</option>
	<option value="SD">South Dakota</option>
	<option value="TN">Tennessee</option>
	<option value="TX">Texas</option>
	<option value="UT">Utah</option>
	<option value="VT">Vermont</option>
	<option value="VA">Virginia</option>
	<option value="WA">Washington</option>
	<option value="WV">West Virginia</option>
	<option value="WI">Wisconsin</option>
	<option value="WY">Wyoming</option>
</select></div>
)");
}



#endif
