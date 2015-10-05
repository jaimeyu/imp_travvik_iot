// Bosch 180 class from https://gist.github.com/feesta/926869ea3e3d373ca76c
// My fork @ https://gist.github.com/jaimeyu/62a3292a703e50689888
// TODO: Fix any bugs as a thank you for the ground work?
class BMP180_Sensor
{
    // Squirrel Class for Bosch BMP180 Temperature and Pressure Sensor
    // [http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf]
    // As used on the Adafruit BMP180 breakout board
    // [http://www.adafruit.com/products/1603]
    // Bus: I2C
    
    // Note: current version only accounts for an oversampling_setting = 0
    
    // Constants for BMP180
 
    static BMP180_out_msb =     "\xF6"
    static BMP180_out_lsb =     "\xF7"
    static BMP180_ctrl =        "\xF4"
    static BMP180_chipid =      "\xD0"
    static BMP180_temp =        "\x2E"
    static BMP180_press =       "\x34"
    static BMP180_eprom_AC1 =   "\xAA"
    static BMP180_eprom_AC2 =   "\xAC"
    static BMP180_eprom_AC3 =   "\xAE"
    static BMP180_eprom_AC4 =   "\xB0"
    static BMP180_eprom_AC5 =   "\xB2"
    static BMP180_eprom_AC6 =   "\xB4"
    static BMP180_eprom_VB1 =   "\xB6"
    static BMP180_eprom_VB2 =   "\xB8"
    static BMP180_eprom_MB =    "\xBA"
    static BMP180_eprom_MC =    "\xBC"
    static BMP180_eprom_MD =    "\xBE"
    
    // Currently only works with oversampling_setting = 0
    static BMP180_OSS = 0;
    
    // Callibration values to be read at initialization
    // from BMP180's on-board EPROM
    
    _ac1 = 0
    _ac2 = 0
    _ac3 = 0
    _ac4 = 0
    _ac5 = 0
    _ac6 = 0
    _vb1 = 0
    _vb2 = 0
    _mb = 0
    _mc = 0
    _md = 0
    
    _b5 = 0
    
    temperature = 0
    
    // I2C values
    
    _i2c_address = 0x77
    _i2c = null
    
    constructor (imp_i2c_bus, i2c_address_7_bit)
    {
        // default i2c_address_7_bit should be 0x77
        _i2c = imp_i2c_bus
        _i2c.configure(CLOCK_SPEED_400_KHZ);
        _i2c_address = i2c_address_7_bit << 1
    }
    
    function init()
    {
        // example values from documentation are commented out
        
        // ac1, ac2, ac3 are signed 16-bit values, so need to be sign-extended
        // to the 32-bit values Squirrel uses
 
        local a = _i2c.read(_i2c_address, BMP180_eprom_AC1, 2);
        _ac1 = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_ac1 = 408;
        a = _i2c.read(_i2c_address, BMP180_eprom_AC2, 2);
        _ac2 = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_ac2 = -72;
        a = _i2c.read(_i2c_address, BMP180_eprom_AC3, 2);
        _ac3 = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_ac3 = -14383;
        
        // Read in per-chip callibration data for temperature conversions
        // ac4, ac5, and ac6 are unsigned 16-bit values
        
        a = _i2c.read(_i2c_address, BMP180_eprom_AC4, 2);
        _ac4 = (a[0] << 8) + a[1];
        //_ac4 = 32741;
        a = _i2c.read(_i2c_address, BMP180_eprom_AC5, 2);
        _ac5 = (a[0] << 8) + a[1];
        //_ac5 = 32757;
        a = _i2c.read(_i2c_address, BMP180_eprom_AC6, 2);
        _ac6 = (a[0] << 8) + a[1];
        //_ac6 = 23153;
 
        // mc and md are signed 16-bit values, so need to be sign-extended
        // to the 32-bit values Squirrel uses
 
        a = _i2c.read(_i2c_address, BMP180_eprom_VB1, 2);
        _vb1 = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_vb1 = 6190;
        a = _i2c.read(_i2c_address, BMP180_eprom_VB2, 2);
        _vb2 = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_vb2 = 4;
        a = _i2c.read(_i2c_address, BMP180_eprom_MB, 2);
        _mb = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_mb = -32768;
        a = _i2c.read(_i2c_address, BMP180_eprom_MC, 2);
        _mc = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_mc = -8711;
        a = _i2c.read(_i2c_address, BMP180_eprom_MD, 2);
        _md = (((a[0] << 8) + a[1]) << 16) >> 16;
        //_md = 2868;

    }
 
    function get_temp()
    {
        // Get the current temperature data, and convert 
        // from internal units to degrees Celsius
        
        // Signal BMP180 to take a reading
        
        _i2c.write(_i2c_address, BMP180_ctrl + BMP180_temp)
        
        // Pause 5ms while chip's ADC digitises the reading
        
        imp.sleep(0.005)
        
        // Get the reading
        
        local a = _i2c.read(_i2c_address, BMP180_out_msb, 2)
        
        // Convert to Celsuis
        
        local ut = (a[0] << 8) + a[1]
        //local ut = 27898;
        //server.log("ut: " + ut + " (27898)");
        local x1 = ((ut - _ac6) * _ac5 / 32768.0);
        //server.log("x1: " + x1 + " (4743)");
        local x2 = (_mc * 2048 / (x1 + _md));
        //server.log("x2: " + x2 + " (-2344)");
        _b5 = x1 + x2
        //server.log("b5: " + _b5 + "(2399)");
        //temperature = round((_b5 + 8) / 16);
        temperature = (_b5 + 8) / 16; // Removed round
        //server.log(">>> temp: " + temperature / 10.0 + "C (15.0C)");
        
        return temperature
    }
    function get_fahrenheit() {
        // read temperature if it's not yet read
        if (temperature == 0) get_temp();
        return temperature * 0.1 * 1.8 + 32.0;
    }
    function get_pressure() {
        // read temperature if it's not yet read
        if (temperature == 0) get_temp();
        _i2c.write(_i2c_address, BMP180_ctrl + BMP180_press)
        imp.sleep(0.005)
        
        local a = _i2c.read(_i2c_address, BMP180_out_msb, 3)
        local pu = ((a[0] << 16) + (a[1] << 8) + (a[2])) >> (8 - BMP180_OSS);
        //local pu = 23843;
        //server.log("pu: " + pu + " (23843)");

        local b6 = _b5 - 4000;
        //server.log("b6: " + b6 + " (-1601)");
        local x1 = round((_vb2 * (b6 * b6 * math.pow(2, -12))) * math.pow(2, -11));
        //server.log("x1: " + x1 + " (1)");
        local x2 = round(_ac2 * b6 * math.pow(2, -11));
        //server.log("x2: " + x2 + " (56)");
        local x3 = x1 + x2;
        //server.log("x3: " + x3 + " (57)");
        local b3 = (((_ac1 * 4 + x3) << BMP180_OSS) + 2) / 4;
        //server.log("b3: " + b3 + " (422)");
        
        x1 = round(_ac3 * b6 * math.pow(2,-13));
        //server.log("x1: " + x1 + " (2810)");
        x2 = round((_vb1 * (b6 * b6 * math.pow(2, -12))) * math.pow(2, -16));
        //server.log("x2: " + x2 + " (59)");
        x3 = round((x1 + x2 + 2) / 4);
        //server.log("x3: " + x3 + " (717)");
        local b4 = _ac4 * (x3 + 32768) * math.pow(2, -15);
        //server.log("b4: " + b4 + " (33457)");
        local b7 = (pu - b3) * (50000 >> BMP180_OSS);
        //server.log("b7: " + b7 + " (1171050000)");
        local p = b7 < 0x80000000 ? b7 * 2 / b4 : (b7 / b4) * 2;
        //server.log("p: " + p + " (70003)");
        x1 = (p / math.pow(2,8)) * (p / math.pow(2,8));
        //server.log("x1: " + x1 + " (74529)");
        x1 = (x1 * 3038) / math.pow(2,16)
        //server.log("x1: " + x1 + " (3454)");
        x2 = (-7357 * p) / math.pow(2, 16);
        //server.log("x2: " + x2 + " (-7859)");
        p = p + (x1 + x2 + 3791) / math.pow(2,4);
        //server.log(">>> p: " + p + " pascals (69964)");
        return p;

    }
    function to_inhg(pascal) {
        // convert pascals to inches of mercury
        return pascal * 0.29529980164712 / 1000;
    }
    
      
    function round(n) {
        return (n < 0) ? (n - 0.5).tointeger() : (n + 0.5).tointeger();
    }

}
