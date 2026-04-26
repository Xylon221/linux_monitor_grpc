import asyncio
import os
import sys
from contextlib import asynccontextmanager

import grpc.aio
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles
from google.protobuf import empty_pb2
from google.protobuf.json_format import MessageToDict

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "pb"))
import monitor_info_pb2_grpc

GRPC_ADDRESS = os.environ.get("GRPC_SERVER", "localhost:50051")


@asynccontextmanager
async def lifespan(app: FastAPI):
    channel = grpc.aio.insecure_channel(GRPC_ADDRESS)
    app.state.channel = channel
    app.state.stub = monitor_info_pb2_grpc.GrpcManagerStub(channel)
    yield
    await channel.close()


app = FastAPI(title="Linux Monitor", lifespan=lifespan)

static_dir = os.path.join(os.path.dirname(__file__), "static")
app.mount("/static", StaticFiles(directory=static_dir, html=True), name="static")


@app.get("/")
async def root():
    with open(os.path.join(static_dir, "index.html")) as f:
        return HTMLResponse(f.read())


@app.get("/api/monitor")
async def get_monitor():
    try:
        data = await app.state.stub.GetMonitorInfo(empty_pb2.Empty())
        return MessageToDict(data, preserving_proto_field_name=True)
    except grpc.aio.AioRpcError:
        return {"error": "gRPC server unavailable"}


@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            try:
                data = await app.state.stub.GetMonitorInfo(empty_pb2.Empty())
                await ws.send_json(
                    MessageToDict(data, preserving_proto_field_name=True)
                )
            except grpc.aio.AioRpcError:
                await ws.send_json({"error": "gRPC server unavailable"})
            await asyncio.sleep(2)
    except WebSocketDisconnect:
        pass


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=8000)
