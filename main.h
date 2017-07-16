struct Sensor {
  unsigned long usage;
  unsigned long count;
  unsigned long oldCount;
  unsigned long meter;
  unsigned long lastPulse;
  unsigned long factor;
  unsigned long deBounce;
  unsigned long reset;
  const char* id;
  const char* topicUsage;
  const char* topicMeter;
  const char* topicCounter;
};

