package de.fraunhofer.itwm.hexabus;

import java.io.IOException;
import java.net.MulticastSocket;
import java.net.SocketException;
import java.net.DatagramPacket;
import java.net.InetAddress;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.LinkedBlockingQueue;


public class HexabusServer {

	private HexabusServerRunnable serverRunnable;
	private Thread parserThread;
	private HashMap<Hexabus.HexabusListener,HexabusListenerEntry> listeners;
	private LinkedBlockingQueue<DatagramPacket> parseQueue;
	private boolean running = false;
	private int port = Hexabus.PORT;

	/**
	 * HexabusServer that listens on port { @value Hexabus#PORT }
	 */
	public HexabusServer() {
		parseQueue = new LinkedBlockingQueue<DatagramPacket>();
		listeners = new HashMap<Hexabus.HexabusListener,HexabusListenerEntry>();
	}

	/**
	 * Starts the server
	 */
	public void start() {
		running = true;
		serverRunnable = (new HexabusServerRunnable(this));
		(new Thread(serverRunnable)).start();
		parserThread = new Thread(new HexabusServerParserRunnable());
		parserThread.start();
	}

	/**
	 * Shuts the server down
	 */
	public void shutdown() {
		running = false;
		serverRunnable.shutdown();
		parserThread.interrupt();
		for(Map.Entry<Hexabus.HexabusListener,HexabusListenerEntry> listener : listeners.entrySet()) {
			listener.getValue().getThread().interrupt();
		}
	}

	/**
	 * Registers a listener with the HexabusServer
	 * @param listener The listener that should be registered
	 */
	public void register(Hexabus.HexabusListener listener) {
		listeners.put(listener, new HexabusListenerEntry(listener));
	}

	/**
	 * Unregisters a listener from the HexabusServer
	 * @param listener The listener that should be unregistered
	 */
	public void unregister(Hexabus.HexabusListener listener) {
		listeners.remove(listener);
	}

	protected void handlePacket(DatagramPacket packet) {
		try {
			parseQueue.put(packet); // Put the packet in the queue for the parser thread
		} catch(InterruptedException e) {
			//TODO rethrow error
		}
			
	}

	private class HexabusListenerEntry {
		private LinkedBlockingQueue<HexabusPacket> queue;
		private Thread thread;

		public HexabusListenerEntry(Hexabus.HexabusListener listener) {
			queue = new LinkedBlockingQueue<HexabusPacket>();
			thread = new Thread(new HexabusServerNotifierRunnable(listener,queue));
			thread.start();
		}

		public LinkedBlockingQueue<HexabusPacket> getQueue() {
			return queue;
		}

		public Thread getThread() {
			return thread;
		}

	}

	private class HexabusServerNotifierRunnable implements Runnable {
		private Hexabus.HexabusListener listener;
		private LinkedBlockingQueue<HexabusPacket> queue;

		public HexabusServerNotifierRunnable(Hexabus.HexabusListener listener, LinkedBlockingQueue<HexabusPacket> queue) {
			this.listener = listener;
			this.queue = queue;
		}

		public void run() {
			while(running) {
				try {
					HexabusPacket packet = queue.take(); //Takes packet from the queue, waits if empty
					listener.handlePacket(packet);
				} catch(InterruptedException e) {
					//TODO rethrow
				}
			}
		}
	}

	private class HexabusServerParserRunnable implements Runnable {

		public HexabusServerParserRunnable() {
		}

		public void run() {
			while(running) {
				try {
					DatagramPacket packet = parseQueue.take(); // Takes packet from the queue, waits if empty
					HexabusPacket parsedPacket = Hexabus.parsePacket(packet);

					for(Map.Entry<Hexabus.HexabusListener,HexabusListenerEntry> entry : listeners.entrySet()) {
						entry.getValue().getQueue().put(parsedPacket);
					}
				} catch(InterruptedException e) {
					if(!running) {
						break;
					}
				} catch(Hexabus.HexabusException e) {
					if(e.getMessage().equals("Unknown packet type")) { //TODO: Don't identify exceptions by their message
						System.err.println("Unknown packet type. Packet dropped.");
						continue;
					} else {
						System.err.println("unknown error");
						continue;
					}
				}
			
			}
		}
	}

			

	private class HexabusServerRunnable implements Runnable {

		private HexabusServer server;
		private MulticastSocket socket;

		public HexabusServerRunnable(HexabusServer server) {
			this.server = server;
		}

		public void shutdown() {
			try {
				socket.leaveGroup(InetAddress.getByAddress(Hexabus.MULTICAST_GROUP));
			} catch(IOException e) {
				//TODO handle IOExceptions
				e.printStackTrace();
			}
			socket.close();
		}

		public void run() {
			try {
				socket = new MulticastSocket(port);
				socket.joinGroup(InetAddress.getByAddress(Hexabus.MULTICAST_GROUP));
				while(running) {

					byte[] data = new byte[141]; // Largest packet: 128string info/write packet
					DatagramPacket packet;
					packet = new DatagramPacket(data, data.length);

					socket.receive(packet);
					server.handlePacket(packet);
				}
				socket.leaveGroup(InetAddress.getByAddress(Hexabus.MULTICAST_GROUP));
				socket.close();
			} catch(SocketException e) {
				if(e.getMessage().equals("Socket closed")) { // urgh..
					return;
				} else {
					e.printStackTrace();
				}
			} catch(IOException e) {
				//TODO handle IOExceptions
				e.printStackTrace();
			}
		}
	}

}
