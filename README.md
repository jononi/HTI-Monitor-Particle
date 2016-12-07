# HTI-Monitor-Particle

Humidity, temperature and ambient light (illumination) monitoring through the cloud

The purpose of this project is to monitor the effects of using a humidifier on the temperature and humidity of a room while the heating system is active. The ambient light monitoring is used to check whether external conditions (sunny/cloudy, day/night) have any effect on the temperature or humidity of the room.

Hardware side I use Particle Photon Wifi enabled micro-controller.
Temperature and humidity sensor is the [HTU21D](https://www.adafruit.com/products/1899) and the light sensor is  [TSL2561](https://www.adafruit.com/products/439), both from Adafruit.

Software side, I use [Romain MP's library](https://github.com/romainmp/HTU21D) for HTU21D, my own TSL2561 library that is a combination from Adafruit's and Sparkfun's libraries to have the best features from both, such as auto gain.
For uploading data using tcp requests, I use a modified http client library by Mojtaba Cazi that is faster and more robust than the standard one.

Finally for cloud service I use Particle cloud services to configure and view the program settings and the free tier service of Ubidots to upload and view the data on their cloud dashboard.

_______________________
The next upgrade for this project is to make it a home environment monitoring. I will soon add a PIR sensor to detect human presence in the room and a sound detector with the long term objective of being able to identify the people in the room based on tehir voice.
I'll be using a standard PIR sensor from eBay and the [sound detector](https://www.sparkfun.com/products/12642) is from the Sparkfun.
