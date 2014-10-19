import java.net.{ServerSocket, Socket}

import scala.io._
import scala.concurrent._

import resource._
import ExecutionContext.Implicits.global

import com.rabbitmq.client.ConnectionFactory
import com.rabbitmq.client.{Connection => RabbitMQConnection, Channel => RabbitMQChannel}
import com.rabbitmq.client.MessageProperties;

object EnQueueServer {

  val QueueName = "rawDataLine"

  def initRabbitConnection() = {
    val factory = new ConnectionFactory
    factory.setHost("localhost")
    factory.newConnection()
  }

  def initRabbitChannel(connection: RabbitMQConnection) = {
    val channel = connection.createChannel()
    channel.queueDeclare(QueueName, true, false, false, null)
    channel
  }

  def processInput(socket: Socket, channel: RabbitMQChannel, counter: Long) {
    for {
      inputStream <- managed(socket.getInputStream)
      bufferedSource <- managed(new BufferedSource(inputStream))
    } {
      val line = bufferedSource.getLines().next()

      println(s"[$counter] EnQueue: $line")

      channel.basicPublish(
        "", QueueName, MessageProperties.PERSISTENT_TEXT_PLAIN, 
        line.getBytes
      )
    }
  }

  def main(args: Array[String]) {

    var counter = 0L

    for {
      server <- managed(new ServerSocket(5566))
      rabbitConnection <- managed(initRabbitConnection())
      channel <- managed(initRabbitChannel(rabbitConnection))
    } {

      while (true) {
        val socket = server.accept()
        Future {
          processInput(socket, channel, counter)
        }
        counter += 1
      }
    }
  }
}
