/**
 * @file MisBKit.cpp
 * @author Etienne Montenegro
 * @brief Implementation file for MisBKit.h
 */

#include "MisBKit.h"
#include "Led.h"

namespace {
  static uint32_t update_fps_last_report_ms = 0;
  static uint32_t update_fps_frame_count = 0;
  static constexpr uint32_t UPDATE_FPS_REPORT_INTERVAL_MS = 10000;

  static void report_update_fps() {
    update_fps_frame_count++;
    uint32_t now = millis();
    if (update_fps_last_report_ms == 0) {
      update_fps_last_report_ms = now;
      return;
    }

    uint32_t elapsed = now - update_fps_last_report_ms;
    if (elapsed >= UPDATE_FPS_REPORT_INTERVAL_MS) {
      static u8_t count = 0;
      
      float fps = update_fps_frame_count * 1000.0f / elapsed;
      if(count >= 10){
        Log.infoln("update() fps: %F", fps);
        count = 0;
      }
      
      update_fps_frame_count = 0;
      update_fps_last_report_ms = now;
      count++;
    }
  }
}


void MisBKit::initialize(bool _clear) {
  Log.infoln("Starting MisBKit initialization");
  led::init();
  // if(_clear) config_manager.clear();
 
  if(config_manager.initialize()){
    load_configurations();
    config_manager.close();
  }
  
  uint8_t id = net_config.ip[3];
  network_manager.update_ap_credentials(id);
  network_manager.initialize();

  Log.warningln("network initialized");
  led::setColor(255,165,0, true);
  ws.setConnectionHandler([this](IPAddress ip) { this->pair(ip); });
  ws.setMessageHandler([this](const Command& cmd){this->process_command(cmd);});
  ws.setDisconnectionHandler([this](){this->disconnect();});
  ws.initialize();

  web_interface.initialize();
  server.begin();
  
  sensor_manager.initialize();

  motor_manager.initialize();
  rc_manager.initialize(rc_config);

  // // // battery pin configuration
  // // pinMode(pins::battery, INPUT);

  ws.setMotorManager(&motor_manager);
  resetCurve();

  led::blinkIP(net_config.ip);
  // led::blinkColor(0,255,0,200,10);
  // led::setColor(0,255,0);
  Log.infoln("MisBKit initialization done.");
}

void MisBKit::load_configurations(){
  if(!config_manager.load())
  { 
    led::error();
    return;
  }

  JsonDocument& doc = config_manager.get_doc();
  kit_config.set_from_json(doc);
  Log.infoln("%p", kit_config);
  net_config.set_from_json(doc);
  Log.infoln("%p", net_config);
  // sensor_config.set_from_json(doc);
  // Log.infoln("%p", sensor_config);
  // rc_config.set_from_json(doc);
  // Log.infoln("%p", rc_config);
  sensor_config.updated = false;
}

void MisBKit::save_configuration(){
  kit_config.add_to_json(config_manager.get_doc());
  net_config.add_to_json(config_manager.get_doc());
  sensor_config.add_to_json(config_manager.get_doc());
  rc_config.add_to_json(config_manager.get_doc());
  if(config_manager.save())
    Log.infoln("Successfully savec configuration");
}


void MisBKit::execute_action(JsonObject action){

    switch (Command::to_enum(action)){
      case CMD_REBOOT:
        reboot_flag = true;
        break;
  
      case CMD_SCAN:
        scan_flag = true;
        break;

  
      case CMD_RC_STOP:{

        if (action["id"].isNull()) { //TEST AND ASK YOURSELF IF THIS IS REALLY THE BEHAVIOR WANTED
          rc_manager.stopAll();
        } else {
          rc_manager.stop(action["id"].as<uint8_t>());
        }
        break;
      }
      case CMD_STOP: {
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr) m->stop();
        break;
      }
  
      case CMD_STOP_ALL: {
        motor_manager.stopAll();
        break;
      }



      case CMD_MOTOR_ID:{
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr){
            m->setID(action["val"].as<int>()); 
        }
        break;
      }

      case CMD_MOTOR_FACTORY_RESET:{
        motor_manager.factory_reset();
        break;
      }

      case CMD_DEEP_SCAN:{
        motor_manager.reconfigureBadBaud();
        scan_flag = true;

        break;
      }

      case CMD_CHANGE_MOTOR_BAUD:{
        motor_manager.setControlBaud( action["val"].as<int>() );
        motor_manager.reconfigureBadBaud();
        scan_flag = true;

        break;
      }

      case CMD_MOTOR_CURVE:{
        Log.infoln("Handling cmd_curve");
        auto id = action["id"].as<uint8_t>();
        auto loop_mode = action["loop"].as<uint8_t>();
        auto duration = action["duration"].as<float_t>();
        if (action["val"].is<JsonArray>()) {
            JsonArray vals = action["val"];
            int16_t * pf = getCurveBuffer(id);
            int i = 0;
            for (int16_t c : vals) {
                int16_t v = c;
                Serial.print("v: ");
                Serial.println(v);
                pf[i] = v;
                i += 1;
            }
            playCurve(id, i,loop_mode,duration);
        }

        break;
      }

      case CMD_MOTOR_CURVE_STOP:{
        auto id = action["id"].as<uint8_t>();
        stopCurve(id);
        break;
      }

      case CMD_MOTOR_POSITION:{
        sendMotorsPosition();
      }

      case CMD_SET_MODE:{
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr){
           if(action["val"].as<int>()){
            m->setMode(WHEEL_MODE);
           }else{
            m->setMode(JOINT_MODE);
           }
        }
        break;
      }

      case CMD_SET_STIFF:{
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr){
           if(action["val"].as<int>()){
            m->setTorqueOn();
           }else{
            m->setTorqueOff();
           }
        }
        break;
      }
      
      case CMD_RC_ENABLE:{
        if (action["id"].is<JsonArray>()) { //MOVE A GROUP OF MOTOR
        }else{
          Serial.println("RC_ENABLE");
          uint8_t id = action["id"];
          uint8_t val = action["val"];
          rc_config.channel_enabled[id] = val;
          rc_manager.activate(id, val);
          JsonDocument reply;
          rc_config.add_to_json(reply);
          ws.reply("rc_config", reply);
        }
        break;
      }
      case CMD_RC_JOINT:{
        if (action["id"].is<JsonArray>()) { //MOVE A GROUP OF MOTOR
          
          break;
        }
        //MMOVE A SINGLE MOTOR
        Serial.println("RC_JOINT");
        const uint8_t channel = action["id"].as<uint8_t>();
        const float target_angle = action["val"].as<float>();
        const uint16_t speed_override = action["speed"] | 0U;
        if (!rc_manager.moveJoint(channel, target_angle, speed_override)) {
          Log.warningln("RC joint move ignored for channel %u", static_cast<unsigned>(channel));
        }
        break;
      }

      case CMD_WHEEL: {
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr) m->setWheelSpeed(action["val"].as<int32_t>());
        break;
      }
  
      case CMD_JOINT: {
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr) m->setGoalPosition(action["val"].as<int32_t>());
        break;
      }
  
      case CMD_RC_JOINT_VELOCITY:{
        if (action["id"].is<JsonArray>()) {
          JsonArray ids = action["id"].as<JsonArray>();
          const uint16_t speed_dps = action["val"].as<uint16_t>();
          for (JsonVariant channel_var : ids) {
            const uint8_t channel = channel_var.as<uint8_t>();
            if (!rc_manager.setJointSpeed(channel, speed_dps)) {
              Log.warningln("RC joint speed update ignored for channel %u", static_cast<unsigned>(channel));
            }
          }
          break;
        }

        const uint8_t channel = action["id"].as<uint8_t>();
        const uint16_t speed_dps = action["val"].as<uint16_t>();
        if (!rc_manager.setJointSpeed(channel, speed_dps)) {
          Log.warningln("RC joint speed update ignored for channel %u", static_cast<unsigned>(channel));
        }
        break;
      }

      case CMD_JOINT_VELOCITY: {
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr) m->setJointSpeed(action["val"].as<int32_t>());
        break;
      }
  
      case CMD_STATUS_DUMP: {
        Motor *m = motor_manager.get(action["id"].as<uint8_t>());
        if (m != nullptr) m->updateInfos();
        break;
      }
  
      case CMD_SENSOR_CONFIG: {
        JsonDocument doc = action["val"];
        if(sensor_manager.set_config(doc)){
          JsonDocument reply;
          sensor_manager.write_to_json(reply);
          ws.reply("sensorconfig",reply);
        
        }
        // sensor_config.set_from_json(doc);
        break;
      }
      
      case CMD_SENSOR_DATA:{
      if(!sensor_manager.no_sensor_enabled()){
          JsonDocument sensor_data;
          sensor_manager.add_data_to_json(sensor_data);
          ws.reply("sensordata",sensor_data);
        } 
      }
        break;

      case CMD_SENSOR_SCAN: {
        uint8_t port_id = action["id"];
        if(sensor_manager.scan(port_id)){
          JsonDocument reply;
          sensor_manager.write_to_json(reply);
          // rc_config.add_to_json(reply);
          ws.reply("sensorconfig", reply);
        }
        
        break;
      }
  
      case CMD_SAVE_SENSOR_CONFIG:
        Log.traceln("Saving sensor config");
        save_configuration();
        network_manager.remove_wifi_events();  // prevent going in the reconnecting loop
        reboot_flag = true;
        break;

      case CMD_SYNC_WHEEL:{
        const uint8_t len = action["id"].size();
        uint8_t id_list[len];
        copyArray(action["id"],id_list,len);
        motor_manager.set_wheel_speed_sync(id_list,len,action["val"].as<uint16_t>());
        break;
      }

      case CMD_SYNC_JOINT:{
        const uint8_t len = action["id"].size();
        uint8_t id_list[len];
        copyArray(action["id"],id_list,len);
        motor_manager.set_goal_position_sync(id_list,len,action["val"].as<uint16_t>());
        break;
      }

      case CMD_SYNC_JOINT_VELOCITY:{
        const uint8_t len = action["id"].size();
        uint8_t id_list[len];
        copyArray(action["id"],id_list,len);
        motor_manager.set_joint_speed_sync(id_list,len,action["val"].as<uint16_t>());
        break;
      }
      
      case CMD_SYNC_STOP:{
        const uint8_t len = action["id"].size();
        uint8_t id_list[len];
        copyArray(action["id"],id_list,len);
        motor_manager.stop_sync(id_list,len);
        break;
      }

      case CMD_RC_INFO:{
        
            break;
      }
      case CMD_FORMAT_SPIFFS:{
        Log.infoln( "WRN: Formatting SPIFFS!" );
        SPIFFS.format(); // efface tout les spiffs !
        break;
      }
      default:
        Log.errorln("Command does not exist");
        break;
    }
}


void MisBKit::process_command(const Command& c) {
  // Log.traceln("Processing command:");
  JsonDocument doc;
  if (c.is_valid(doc)) {
    // Log.traceln("Command is valid");
    if (c.is_single_command(doc)) {
      // Log.traceln("Executing single command");
      execute_action(doc.as<JsonObject>());
    } else {
      // Log.traceln("Executing multiple commands");
      if (doc["cmds"].is<JsonArray>()) {
        for (int i = 0; i < doc["cmds"].size(); i++) {
          // Log.traceln("Executing command %d/%d", i+1, doc["cmds"].size());
          execute_action(doc["cmds"][i]);
        }
      }
    }
  } else {
    Log.errorln("Command is invalid");
  }
}


void MisBKit::update() {
  if( WebInterface::isReceivingUpload() )
  {
    // do nothing while upload to prevent uploading to fail (when too slow, packet are lost)
    //Serial.println( "MisBKit: skipping update...");

    return;
  }
  ws.update();
  sensor_manager.update();

  if (scan_flag) {
    uint8_t found = motor_manager.scanMotors();
    Log.info("Found %d motor during scan", found);
    JsonDocument doc;
    doc["ids"].to<JsonArray>();
    if (found > 0) {

      for (Motor *m : motor_manager.motors()) {
        if (m) doc["ids"].add<int>(m->getID());
      }
    }

    ws.reply("scan",doc);
    resetCurve();
    scan_flag = false;
  }

  if (reboot_flag) {
    reboot();
  }
  led::update();
  // report_update_fps();

  uint32_t current_time = millis();
  static uint32_t last_update_curve = 0;
  if (current_time - last_update_curve > 40 && !scan_flag ) { // 25 fps
      last_update_curve = current_time;
      update_curve_play();
  }

  static uint32_t last_send_position = 0;
  if (current_time - last_send_position > 100  && !scan_flag ) {
      last_send_position = current_time;
      sendMotorsPosition();
  }


  

// #if defined(PCB)
//   // MisBKit::readBattery(60);
//   MisBKit::led::update();
// #endif
}


void MisBKit::pair(IPAddress ip) {
  Log.traceln("Trying to pair mcu");

  MisBKit::connected = true;
  
  JsonDocument reply;
  sensor_manager.write_to_json(reply);
  // sensor_config.add_to_json(reply);
  ws.reply("sensorconfig", reply);
  reply.clear();
  rc_config.add_to_json(reply);
  ws.reply("rc_config", reply);
  
  JsonDocument mess;
  mess["ip"] = ip.toString().c_str();
  mess["version"] = kit_config.v_as_str();
  ws.reply("pair", mess);

  Log.traceln("End of pairing");
}

void MisBKit::reboot() { 
  Log.warningln("Rebooting");
  rc_manager.shutdown();
  delay(1000);
  ESP.restart(); 
}

void MisBKit::sendMotorsPosition() {
  //Log.infoln("ws::sendMotorsPosition");
  JsonDocument doc;
  doc["pos"].to<JsonArray>();
  for (Motor *m : motor_manager.motors()) {
        doc["pos"].add<int>(m->getActualPosition());
  }
  ws.reply("positions",doc); // < 3ms for 5 motors
}

void MisBKit::resetCurve()
{
  // attention sur les memset sur des entiers...
  //memset(anPosCurve_, 0xFF, WS_NBR_MAX_MOTORS*sizeof(int));
  for( int i = 0; i < WS_NBR_MAX_MOTORS; ++i )
  {
    arPosCurve_[i] = -1.f;
  }
  memset(anLenCurve_, 0xFF, WS_NBR_MAX_MOTORS*sizeof(int));
  //memset(anIncCurve_, 1, WS_NBR_MAX_MOTORS*sizeof(int)); // ne met pas 0x00000001 mais 0x01010101 => ne rien faire on le mettra dans le play
  memset(anLoopMode_, 0, WS_NBR_MAX_MOTORS*sizeof(int));
  memset(abIsPlaying_, 0, WS_NBR_MAX_MOTORS*sizeof(bool));
}


int16_t * MisBKit::getCurveBuffer( int id_motor )
{ 
  Serial.println("INF: getCurveBuffer");
  return aaCurvePos_[ motor_manager.getMotorIndex(id_motor)]; 
}




void MisBKit::playCurve( int id_motor, int len_curve, int loop_mode, float duration )
{
  Serial.println("INF: playCurve");
  if( duration < 0 )
  {
    duration = 1.f; // can't go faster
  }
  int idx =  motor_manager.getMotorIndex(id_motor);
  anLenCurve_[idx] = len_curve;
  arPosCurve_[idx] = 0.f;
  abIsPlaying_[idx] = true;
  arIncCurve_[idx] = +9.f/duration;
  anLoopMode_[idx] = loop_mode;

  Serial.print( "playing: id_motor: " );
  Serial.print( id_motor );
  Serial.print( ", idx: " );
  Serial.print( idx );
  Serial.print( ", len_curve: " );
  Serial.print( len_curve );
  Serial.print( ", duration: " );
  Serial.println( duration );

  Motor *m = motor_manager.get(id_motor);
  //m->changeStatusReturnLevel(false);
}

void MisBKit::stopCurve( int id_motor )
{
  Log.infoln("INF: stopCurve");
  int idx =  motor_manager.getMotorIndex(id_motor);
  arPosCurve_[idx] = -1.f;
  delay(20); // si on est en train d'executer une mise a jour des moteurs, on peut rater cette commande (pas de mutex)
  arPosCurve_[idx] = -1.f;
  // Motor *m = motor_manager.get(id_motor);
  // m->changeStatusReturnLevel(true);
  Log.infoln("INF: stopCurve - ended");
}

#define LOOP_NORMAL 0
#define LOOP_PINGPONG 1
#define LOOP_ONCE 2

void MisBKit::update_curve_play()
{
  //Log.infoln("MisBKit::update_curve_play");
  for( int idx = 0; idx < WS_NBR_MAX_MOTORS; ++idx )
  {
    if( ! abIsPlaying_[idx] )
      continue;

    Motor *m = motor_manager.getFoundMotorIds()[idx];
    if (m != nullptr) 
    {
      float pos = arPosCurve_[idx];
      int posint = int(pos); // pas d'interpolation pour l'instant
      float goal = aaCurvePos_[idx][posint];
      //Serial.print( "DBG: update_curve_play: goal: " );
      //Serial.println( goal );
      // interpolation
      {
        float invratio = pos - posint; // eg: 3.2 => 0.2
        float nextpos = 0;
        if( arIncCurve_[idx] > 0.f || 1 )
        {
          if( posint+1 < anLenCurve_[idx] )
          {
            nextpos = aaCurvePos_[idx][posint+1];
          }
          else
          {
            if( anLoopMode_[idx] == LOOP_NORMAL )
            {
              // interpolation with the next one which is the start of the curb
              nextpos = aaCurvePos_[idx][0];
            }
            else
            {
              nextpos = goal; // interpolation with himself
            }
          }
        }
        else
        {

        }
        goal = goal * (1-invratio) +  nextpos * invratio;
      }

      //Serial.print( "DBG: update_curve_play: goal interp: " );
      //Serial.println( goal );
      //m->setGoalPosition( goal ); // duration: entre 8 et 12ms (Dans mon cas les 3 premiers moteurs prennent 11 et les 2 derniers 8)
      m->setGoalPositionNoRx( goal );
    }
    else
    {
      Serial.print("ERR: update_curve_play: motor not found: ");
      Serial.println(idx);
    }

    if( !abIsPlaying_[idx] )
      continue;

    arPosCurve_[idx] += arIncCurve_[idx];
    if( arPosCurve_[idx] >= anLenCurve_[idx] )
    {
      if( anLoopMode_[idx] == LOOP_PINGPONG )
      {

        arIncCurve_[idx] = -arIncCurve_[idx];
        arPosCurve_[idx] = anLenCurve_[idx] + arIncCurve_[idx];
      }
      else if( anLoopMode_[idx] == LOOP_NORMAL )
      {
        arPosCurve_[idx] -= anLenCurve_[idx];
      }
      else
      {
        // mode once
        arPosCurve_[idx] = 0.f;
        abIsPlaying_[idx] = false;
      }
    }
    if( arIncCurve_[idx] < 0.f && arPosCurve_[idx] <= 0.f )
    {
      // forcément un mode pingpong
      arIncCurve_[idx] = -arIncCurve_[idx];
      arPosCurve_[idx] = 0;
    }

    //Serial.print( "playing: idx: " );
    //Serial.print( idx );
    //Serial.print( ", next pos: " );
    //Serial.println( arPosCurve_[idx] );

  } // fin for
}





// void readBattery(int interval) {
//   static Chrono batteryRead(true);
//   static uint32_t readings[10];
//   static int index{0};
//   static bool firstFill{false};

//   // battery read are a bit bouncy, maybe make a moyenne off all the readings
//   if (batteryRead.hasPassed(interval * 1000, true)) {
//     readings[index] = analogReadMilliVolts(pins::battery);

//     if (firstFill) {
//       uint32_t voltage = 0;

//       for (uint32_t r : readings) {
//         voltage += r;
//       }

//       voltage = voltage / (sizeof(readings) / sizeof(readings[0]));

//       Log.infoln("Battery at %d mv", voltage);

//       if (voltage > 2467) {
//         Log.infoln("Battery is over 50 percent charge");
//       } else {
//         if (voltage < 2100) {
//           Log.warningln("Battery is almost dead. Please change.");

//           MisBKit::led::status = MisBKit::led::status::batteryLow;
//           // todo add led animation to let user know
//           //  todo send osc message to let user know
//         } else {
//           Log.infoln("Battery is under 50 percent charge");
//           MisBKit::led::status = MisBKit::led::status::batteryMedium;
//         }
//       }
//     }

//     index++;
//     if (index == 9 &&
//         !firstFill) {  // allow printing when array is filled for first time
//       firstFill = true;
//     }
//     index = index % 10;
//   }
// }
