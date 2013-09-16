ALUT::init

begin
  src = AL::Source.new
  src.buffer = AL::Buffer.waveform AL::Buffer::WAVEFORM_SINE, 440, 0, 3
  src.play
  ALUT::sleep 3
ensure
  ALUT::exit
end
