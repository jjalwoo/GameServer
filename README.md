# IOCP 기반 에코 Server

## 설명
- Windows IOCP 기반 에코 서버 구현
- 별도 엔진 없이 콘솔에서 동작하는 클라이언트 제공  

## 주요 기술 스택 및 연동
- Winsock2 + IOCP 사용
  
- 데이터베이스  
  - MySQL Connector/C++ (관계형 DB)  
  - Redis++ (sw/redis++) (인메모리 캐시 · Pub/Sub)

- ProtoBuf 연동
  
- JSON 처리  
  - nlohmann/json (경량 C++ JSON 직렬화/파싱 라이브러리)


