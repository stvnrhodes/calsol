#include "pitches.h"

Song::Song(byte buzzer_pin) {
  digitalWrite(buzzer_pin, LOW);
  pinMode(buzzer_pin, OUTPUT);
  buzzer_ = buzzer_pin;
}

void Song::PlaySong(const prog_uint16_t song[][2]){
  #ifdef PITCHES_DEBUG
    Serial.println("New Song");
  #endif
  song_ = song;
  note_start_time_ = millis();
  note_duration_ = 0;
  note_number_ = 0;
}

void Song::KeepPlaying() {
  long current_time = millis();
  if (song_ != 0 && note_start_time_ + note_duration_ <= current_time) {
    #ifdef PITCHES_H_USE_TONE
      noTone(buzzer_);
    #else
      digitalWrite(buzzer_, LOW);
    #endif
    uint16_t note_pitch = song_[note_number_][0];
    note_duration_ = song_[note_number_][1];
    note_start_time_ = current_time;
    ++note_number_;
    if (!note_duration_) {
      #ifdef PITCHES_DEBUG
        Serial.print("Note Pitch: ");
        Serial.println("End Of Song");
      #endif
      song_ = 0;
    } else if (note_pitch) {
      #ifdef PITCHES_DEBUG
        Serial.print("Note Pitch: ");
        Serial.println(note_pitch);
      #endif
      #ifdef PITCHES_H_USE_TONE
        tone(buzzer_, note_pitch);
      #else
        digitalWrite(buzzer_, HIGH);
      #endif
    } else {
       #ifdef PITCHES_DEBUG
        Serial.println("Note Pitch: Silence");
      #endif
    }     
    #ifdef PITCHES_DEBUG
      Serial.print("Note Duration: ");
      Serial.println(note_duration_);
    #endif   
  }
}

bool Song::IsPlaying(void) {
  return song_ != 0;
}
