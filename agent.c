#require "Dweetio.class.nut:1.0.0"

dweet <- DweetIO();


// Basic wrapper to create an execute an HTTP GET
// Will be used to set the device's bus and stop number.
function HttpGetWrapper (url, headers) {
  local request = http.get(url, headers);
  local response = request.sendsync();
  return response;
}


/* Debugging tools */
// HTTP Request handlers expect two parameters:
// request: the incoming request
// response: the response we send back to whoever made the request
function requestHandler(request, response) {
  local res = "Not ok";
  // Check if the variable led was passed into the query
  if ("update_now" in request.query) {
    // if it was, send the value of it to the device
    device.send("update_now", 0);
    res = "<html><head><title>TravvikIoT Imp</title></head><body>Manually updated all data sources.</br> Check <a href='https://freeboard.io/board/rZI0R0'>Freeboard rZI0R0</a></body></html>"
  }
  // send a response back to whoever made the request
  response.send(200, res);
}

// your agent code should only ever have ONE http.onrequest call.
http.onrequest(requestHandler);

server.log(http.agenturl()); // output the agent url to the log console.

bus <- 97;
stop <- 3011;
direction <- 1;
_url <- "http://bus.travvik.com/json/bus/%d/%d";

dst <- "";
eta <- "";

update_cnt <- 0;
old_idx <- 0;

geo_lat <- 45.4062;
geo_long <- -75.7370 ;

function get_latest() {
  local url = format(_url,bus,stop);
  server.log(url);
  local resp = HttpGetWrapper(url, { "Content-Type" : "text/xml" });
  
  //server.log(resp.body);
  local json = split(resp.body, "\n");
  //server.log(json[1]);
  json = http.jsondecode(json[1])
  //server.log(json);
  try {
    dst = json.RouteDirection[direction].RouteNo + " to " + json.RouteDirection[1].Trips.Trip[0].TripDestination;
    eta = json.RouteDirection[direction].Trips.Trip[0].TripAdjustedScheduleTime;
    //eta = "15";
  }
  catch(error){
    server.error("Can't find bus");
    dst = bus + " not in service.";
    eta = "00";
  }
  
  dst.toupper();
  eta.toupper();
  old_idx = 0;
  
}
function update(data) {
  if (dst == "" || eta == "") {
    get_latest();
  }
  update_cnt++;
  if (update_cnt > 30) {
    update_cnt = 0;
    get_latest();
  }
  
  local shdst = "";
  local data = "data";
  const MAX_CHARS = 5;
  // Dst name is long? scroll it! 
  // if (dst.len() > 5) {
  //   local cur_idx = old_idx + MAX_CHARS;
  //   if (cur_idx > dst.len()) {
  //     cur_idx = dst.len();
  //   }
  //   shdst = dst.slice(old_idx, cur_idx);
  // }
  
  //server.log(shdst);
  old_idx++;
  if (old_idx == dst.len() -1) {
    old_idx = 0;
  }
  
  //server.log("len dst: "+ shdst.len()  + " .." + (MAX_CHARS - shdst.len()));
  
  if (shdst.len() < MAX_CHARS) {
    for (local i = MAX_CHARS - shdst.len(); i >0; i--){
      shdst = shdst + " ";
    }
  }
    
  local cur_eta = "0";

  //server.log("cur_eta dst: "+ eta.len()  + " .." + (8 - eta.len()));


  if (eta.len() == 1) {
    cur_eta = "0" + eta;
  }else {
    cur_eta = eta;
  }
    


  //data = format("%s %s", shdst.toupper(), cur_eta);
  server.log("{dst:" + dst  + ",cur_eta:" + cur_eta + "}");
  device.send("eta", cur_eta);
  
  device.send("dst", dst.toupper());
  
  /* Dweet it! */
  dweet.dweet("travvik-next-eta", {"bus" : bus, "dst" : dst, "stop" : stop, "eta": eta, "stop":stop}, function(response) {
    server.log(response.statuscode + ": " + response.body);
  });
}

function on_temp(temp){
  // Send to dweet
  // Asynchronous dweet
  dweet.dweet("travvik-temperature", {"temperature" : temp}, function(response) {
    server.log(response.statuscode + ": " + response.body);
  });
  server.log("Temperature is: " + temp + "°c");
  
  /* Send my GPS location */
  dweet.dweet("travvik-geolocation", {"latitude" : geo_lat, "longitude" : geo_long}, function(response) {
    server.log(response.statuscode + ": " + response.body);
  })
}

function on_pressure(pressure) {
  // Send to dweet
  dweet.dweet("travvik-pressure", {"pressure" : pressure / 1000}, function(response) {
    server.log(response.statuscode + ": " + response.body);
  });
  server.log("Pressure is: " + pressure / 1000 + " kPa");
}

device.on("update", update);
device.on("temp", on_temp);
device.on("pressure", on_pressure);

