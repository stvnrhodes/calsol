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
  KeepPlaying();
}

void Song::KeepPlaying(void) {
  long current_time = millis();
  if (song != NULL && note_start_time_ + note_duration_ <= current_time) {
    noTone(buzzer_);
  	unsigned int note_pitch = pgm_read_word_near(*(song_));
  	note_duration_ = pgm_read_word_near(*(song_) + 1);
    ++song_;
    if (note_duration_ == 0) {
      song_ == NULL;
    } else if (note_pitch != 0) {
      tone(buzzer_, note_pitch)
    }
  }
}

bool Song::IsPlaying(void) {
  return song != NULL;
}
