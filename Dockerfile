# Configure address in client.conf before building the image to make all containers use the same address.
# Otherwise, run the container with the ENV variable ADDRESS, example:
# docker build . -t pow
# docker run -e ADDRESS=xrb_3wm37qz19zhei7nzscjcopbrbnnachs4p1gnwo5oroi3qonw6inwgoeuufdp pow

FROM python:3

RUN pip3 install requests

RUN apt-get update
RUN apt-get install -y libb2-dev

WORKDIR /usr/src/app

COPY . .

RUN autoconf
RUN ./configure
RUN make

CMD python3 client.py || python3 client.py --address $ADDRESS
