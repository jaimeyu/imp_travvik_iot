/* Weather */
// Should be address 0x77 for the bosch 180
weather <- BMP180_Sensor(hardware.i2c12, 0x77);
weather.init(); // Why?
/* End weather */

// Create a global variabled called 'led' and assign the 'pin9' object to it
// The <- is Squirrel’s way of creating a global variable and assigning its initial value
led <- array(1);
led[0] = hardware.pin9;

// Configure 'led' to be a digital output with a starting value of digital 0 (low, 0V)
led[0].configure(DIGITAL_OUT, 1);

spi1 <- hardware.spi257;
spi1.configure(SIMPLEX_TX | MSB_FIRST | CLOCK_IDLE_LOW,  400);


// Create a global variable to store current state of 'led'
state <- 0;

count <-8;

Tab <- [0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff]; //0,1,2,3,4,5,6,7,8,9, ALL OFF
ALPHANUMERIC <- [0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90, 
// These are misc chars, todo?
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
// These are the alphabet
0xA0,0x83,0xa7,0xa1,0x86,0x8e,0xc2,0x8b,0xe6,0xe1,0x89,0xc7,0xaa,0xc8,0xa3,0x8c,0x98,0xce,0x92
,0x87,0xc1,0xe3,0xd5,0x89,0x91,0xb8];//a,b,c,d,e,f,g,h,i,j,k,l,o,m,n,o,p,q,r,s,t,u,v,w,x,y,z
Tap <- [0xff,0x7f]; //"space", "." 

const LED_ALL_OFF = "        ";


eta <- 88;
dst <- "NOT AVAILABLE";
display_string <- "        ";
ticker_pos <- 0;

// 0.25 ms per scroll;
refresh_rate <- 0.35; 

// update every 3 minutes;
update_rate <- (60 * 3) / refresh_rate; 

// decrement the eta every 1 minute;
decrement_rate <- 60 / refresh_rate; 

server.log("r:" + refresh_rate);
server.log("u:" + update_rate);
server.log("d:" + decrement_rate);

function update_eta(data) {
  eta = data.tointeger();
  if (eta > 99) {
    // Display can only show 2 digits...
    // If bus is that late... it's hopeless
    eta = 99;
  }
  if (eta < 0 ) {
    eta = 0;
  }
  ticker_pos = 0;
}

function update_dst(data){
  dst = data + " ";
  if (data.len() < 8) {
    for (local i = 0; i < (8-data.len()); i++) {
      dst = dst + " ";  
    }
    
  }

  ticker_pos = 0;
}

function update_weather(){
  local cur_temp = weather.get_temp() / 10; // in celcius 
  // not sure why the sensor reports celcius * 10...
  // Probably to avoid floating point maths.
  
  local cur_pressure = weather.get_pressure(); // in pascals
  
  agent.send("temp", cur_temp);
  agent.send("pressure", cur_pressure);
}


function blink() {
  
  
  count++;
  /* Debug counter */
  if (count % 10) {
    //server.log("Counter++ : " + count);
  }
  /* End Debug */
  
  if ((count % update_rate) == 0) {
    agent.send("update", 0);
    update_weather();
    count = 0;
    //server.log("Update the count:" + count);
  }
  
  if ((count % decrement_rate) == 0) {
    eta--;
    //server.log("Update the eta:" + eta);
  }
  
  if (eta < 0) {
    eta = 0;
    count = 0;
    agent.send("update", 0);
    update_weather();
    //server.log("Update the count  eta < 0:" + count);
  }
  
  
  display();
  //imp.sleep(0.5);
  
  // Schedule the imp to wakeup in 0.5 seconds and call blink() again 
  imp.wakeup(refresh_rate, blink);
    
}

function create_string() {
  local cur_dst = dst;
  local cur_eta = eta;
  if (dst.len() > 5) {
    local tmp = dst + " " + dst + " " + dst + " " + dst;
    ticker_pos ++;
    ticker_pos = ticker_pos % (dst.len()+2);
    cur_dst = tmp.slice(ticker_pos, ticker_pos+5);
    
  }
  
  if (eta < 10) {
    cur_eta = "0" + eta;
  }
  
  
  local ret = cur_dst + " " + cur_eta; // 8 total
  
  return ret;
}

function display() {
  //server.log("ETA: " + eta + ",DST:" + dst + ".");
  // Build the string
  local data = create_string();


  //server.log("rvcd:"+ data);
  local d = array(8);
  
  //Reverse string
  for (local i=0; i<8; i++) {
    d[7-i] = data[i];
  }
  //server.log("reversed: "+ d);
  data = d;

  //server.log("rvcd: "+ data[2]);
  
  //data = data.reverse();

  //count = (count + 1 ) % (99);
  local blob = blob(8);           // Create a 8-byte blob...
  local val = 0;
  local seg1 = 0;
  local seg2 = 0;
  
  local shift = 0;
  //server.log("Time is: " + count);

  
  foreach(key,val in data.slice(0,4)) {
    //server.log("key:" + key + ",val:" + val);
    if (val == 32) {
      seg1 = seg1 | (0xff << (key*8));

    } else {
      local cur = 0xFF;
      if ( (val - 48) <= ALPHANUMERIC.len() && (val - 48) >=0 ) {
        cur = (0xFF & ALPHANUMERIC[val - 48]);
      }
      seg1 = seg1 | (cur  << (key*8));
    }
  }
  
  foreach(key,val in data.slice(4,8)) {
    //server.log("key:" + key + ",val:" + val);
    if (val == 32) {
      seg2 = seg2 | (0xFF << (key*8));

    } else {
      local cur = 0xFF;
      if ( (val - 48) <= ALPHANUMERIC.len() && (val - 48) >=0 ) {
        cur = (0xFF & ALPHANUMERIC[val - 48]);
      }
      seg2 = seg2 | (cur  << (key*8));
    }
  }


  led[0].write(0);
  //imp.sleep(0.1);
  blob.writen(seg1, 'i');
  blob.writen(seg2, 'i');
  //led[0].write(1);
  //imp.sleep(0.1);

  //led[0].write(0);
  //imp.sleep(0.1);

  spi1.write(blob);
 
  //imp.sleep(0.1);
  led[0].write(1);
  
    
}

function update_now(_) {
  /* Set the count to update rate -1
  to force an update soon
  */
  count = update_rate -1;
}
    
agent.send("update", 0);
agent.on("eta", update_eta);
agent.on("dst", update_dst);
agent.on("update_now", update_now);
// Start the loop
update_weather();
display();
imp.wakeup(0.5, blink);

imp.setpowersave(true);