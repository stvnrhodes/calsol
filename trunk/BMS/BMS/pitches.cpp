#include "pitches.h"

Song::Song(byte buzzer_pin) {
  digitalWrite(buzzer_pin, LOW);
  pinMode(buzzer_pin, OUTPUT);
  buzzer_ = buzzer_pin;
}

void Song::PlaySong(const prog_uint16_t song[][2]){
  song_ = song;
  note_start_time_ = millis();
  note_duration_ = 0;
}

void Song::KeepPlaying() {
  long current_time = millis();
  if (song_ != 0 && note_start_time_ + note_duration_ <= current_time) {
//    noTone(buzzer_);
    unsigned int note_pitch = pgm_read_word_near(*(song_));
    note_duration_ = pgm_read_word_near(*(song_) + 1);
    ++song_;
    if (!note_duration_) {
      song_ = 0;
    } else if (note_pitch) {
//      tone(buzzer_, note_pitch);
    }
  }
}

bool Song::IsPlaying(void) {
  return song_ != 0;
}
