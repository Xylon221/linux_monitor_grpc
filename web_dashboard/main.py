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
    """返回所有主机的监控数据"""
    try:
        all_data = await app.state.stub.GetAllMonitorInfo(empty_pb2.Empty())
        result = {"hosts": []}
        for host_info in all_data.hosts:
            host_dict = MessageToDict(host_info, preserving_proto_field_name=True)
            host_dict["name"] = host_info.name
            result["hosts"].append(host_dict)
        return result
    except grpc.aio.AioRpcError as e:
        return {"error": f"gRPC server unavailable: {e.details()}"}


@app.get("/api/hosts")
async def get_hosts():
    """返回所有主机名列表"""
    try:
        all_data = await app.state.stub.GetAllMonitorInfo(empty_pb2.Empty())
        hostnames = [h.name for h in all_data.hosts if h.name]
        return {"hosts": hostnames}
    except grpc.aio.AioRpcError:
        return {"hosts": []}


@app.get("/api/memory")
async def get_memory():
    """返回内存监控数据"""
    try:
        all_data = await app.state.stub.GetAllMonitorInfo(empty_pb2.Empty())
        memory_data = []
        for host_info in all_data.hosts:
            mem_info = {
                "host": host_info.name,
                "used_percent": host_info.memory.used_percent,
                "total": host_info.memory.total,
                "free": host_info.memory.free,
                "buffers": host_info.memory.buffers,
                "cached": host_info.memory.cached,
            }
            memory_data.append(mem_info)
        return {"memory": memory_data}
    except grpc.aio.AioRpcError as e:
        return {"error": f"gRPC server unavailable: {e.details()}"}


@app.get("/api/network")
async def get_network():
    """返回网络监控数据"""
    try:
        all_data = await app.state.stub.GetAllMonitorInfo(empty_pb2.Empty())
        network_data = []
        for host_info in all_data.hosts:
            for net in host_info.network_interfaces:
                net_info = {
                    "host": host_info.name,
                    "interface": net.name,
                    "send_rate": net.send_rate,
                    "rcv_rate": net.rcv_rate,
                }
                network_data.append(net_info)
        return {"network": network_data}
    except grpc.aio.AioRpcError as e:
        return {"error": f"gRPC server unavailable: {e.details()}"}


@app.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    try:
        while True:
            try:
                all_data = await app.state.stub.GetAllMonitorInfo(empty_pb2.Empty())
                result = {"hosts": []}
                for host_info in all_data.hosts:
                    host_dict = MessageToDict(host_info, preserving_proto_field_name=True)
                    host_dict["name"] = host_info.name
                    result["hosts"].append(host_dict)
                await ws.send_json(result)
            except grpc.aio.AioRpcError:
                await ws.send_json({"error": "gRPC server unavailable"})
            await asyncio.sleep(2)
    except WebSocketDisconnect:
        pass


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="0.0.0.0", port=8000)
