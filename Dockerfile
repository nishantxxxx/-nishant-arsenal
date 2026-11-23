FROM python:3.11-slim

WORKDIR /app

# Copy application files
COPY . .

# Set environment
ENV PORT=8080
ENV PYTHONUNBUFFERED=1

EXPOSE 8080

# Start high-performance video streaming server
CMD ["python", "server.py"]
