# To build the image run:
# docker build . -t pow
# Then run the container with the ENV variable ADDRESS, example:
# docker run -d -e ADDRESS=xrb_3wm37qz19zhei7nzscjcopbrbnnachs4p1gnwo5oroi3qonw6inwgoeuufdp pow

FROM python:3

RUN pip3 install requests

RUN apt-get update
RUN apt-get install -y libb2-dev

WORKDIR /usr/src/app

COPY . .

RUN python3 setup.py build
RUN python3 setup.py install

CMD nano-dpow-client --address $ADDRESS
