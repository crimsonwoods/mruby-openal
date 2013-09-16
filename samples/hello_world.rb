ALUT::init

begin
  src=AL::Source.new
  src.buffer = AL::Buffer.hello_world
  src.play
  ALUT::sleep 2
ensure
  ALUT::exit
end

