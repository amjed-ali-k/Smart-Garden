FROM python:3.10-slim
RUN mkdir /app
WORKDIR /app
ADD backend/requirements.txt /app
RUN pip3 install -r requirements.txt
ADD ./backend /app
EXPOSE 8080
RUN chmod +x ./entrypoint.sh
ENTRYPOINT ["sh", "entrypoint.sh"]