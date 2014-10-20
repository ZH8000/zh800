import java.util.Date
import java.text.SimpleDateFormat
import com.mongodb.casbah.Imports._
import scala.util.Try

case class Record(
  orderType: String, 
  lotNo: String, 
  workQty: Long, 
  countQty: Long,
  embDate: Long, 
  badQty: Long,
  machineIP: String,
  defactID: Long,
  machID: String,
  workID: String,
  cx: String,
  dx: String,
  lc: String,
  machineStatus: String,
  product: String,
  insertDate: String
) {
  def toMongoObject = MongoDBObject(
    "order_type" -> orderType,
    "lot_no" -> lotNo,
    "work_qty" -> workQty,
    "count_qty" -> countQty, 
    "emb_date" -> embDate,
    "bad_qty" -> badQty,
    "mach_ip" -> machineIP,
    "defact_id" -> defactID,
    "mach_id" -> machID,
    "work_id" -> workID,
    "CX" -> cx,
    "DX" -> dx,
    "LC" -> lc,
    "mach_status" -> machineStatus,
    "product" -> product,
    "insertDate" ->  insertDate
  )
}

object Record {

  val dateFormatter = new SimpleDateFormat("yyyy-MM-dd")
  val productMapping = Map(
    "A65"    -> "18",       "A41"    -> "12.5",    "A43"    -> "12.5",     "A42"     -> "12.5",     "A51"    -> "12.5",
    "A40"    -> "12.5",     "A47"    -> "12.5",    "A67"    -> "18",       "A63"     -> "12.5",     "A66"    -> "12.5",
    "A38"    -> "8",        "A48"    -> "18",      "A50"    -> "12.5x31",  "A49"     -> "18",       "A62"    -> "8",
    "A58"    -> "5",        "A52"    -> "12.5",    "A44"    -> "18",       "A57"     -> "10x16",    "A60"    -> "18",
    "A64"    -> "18",       "A54"    -> "18",      "A53"    -> "18",       "A61"     -> "18",       "A55"    -> "18",
    "A59"    -> "18",       "A56"    -> "18",      "A39"    -> "5",        "A30"     -> "8",        "A23"    -> "8",
    "A22"    -> "10",       "A16"    -> "8",       "A15"    -> "8",        "A08"     -> "6",        "A07"    -> "5",
    "A35"    -> "5",        "A35"    -> "5",       "A33"    -> "10",       "A31"     -> "5",        "A29"    -> "6",
    "A28"    -> "10",       "A21"    -> "6",       "A20"    -> "8",        "A14"     -> "6",        "A13"    -> "6",
    "A06"    -> "10",       "A05"    -> "8",       "A36"    -> "5",        "A34"     -> "5",        "A32"    -> "10",
    "A27"    -> "12.5",     "A26"    -> "12.5",    "A19"    -> "10",       "A46"     -> "5",        "A12"    -> "6",
    "A11"    -> "10",       "A04"    -> "5",       "A03"    -> "6",        "A25"     -> "10",       "A24"    -> "10",
    "A18"    -> "12.5",     "A17"    -> "8",       "A10"    -> "10",       "A09"     -> "10",       "A02"    -> "12.5",
    "A01"    -> "8",        "G111"   -> "18x50",   "E116"   -> "12.5x20",  "E117"    -> "12.5x31",  "E118"   -> "18",
    "E119"   -> "18",       "E120"   -> "18",      "E49-S7" -> "18",       "E49-S6"  -> "18",       "E49-S5" -> "18",
    "E49-S4" -> "18",       "E108"   -> "18",      "E49-S3" -> "18",       "E49-K1"  -> "18",       "G111"   -> "18x50",
    "E115"   -> "12.5x25",  "E114"   -> "12.5x25", "E123"   -> "18",       "E122"    -> "18",       "E121"   -> "18",
    "E49-S8" -> "18",       "E49-S1" -> "18",      "E102"   -> "18",       "E101"    -> "18",       "E49-S2" -> "18",
    "E49-K2" -> "18",       "G47"    -> "16",      "G94"    -> "16x25",    "G109"    -> "16",       "G77"    -> "6x11",
    "E77"    -> "6x11",     "G80"    -> "6x11",    "E80"    -> "6x11",     "G75"     -> "6x11",     "E75"    -> "6x11",
    "G81"    -> "6x11",     "E81"    -> "6x11",    "G24"    -> "5x11",     "E24"     -> "5x11",     "G30"    -> "5x11",
    "E30"    -> "5x11",     "G93"    -> "16",      "G108"   -> "18",       "G107"    -> "12.5x31",  "E71"    -> "6x11",
    "G71"    -> "6x11",     "E13"    -> "6x11",    "G13"    -> "6x11",     "E34"     -> "5x11",     "G34"    -> "5x11",
    "E73"    -> "5x11",     "G73"    -> "5x11",    "E79"    -> "6x11",     "G79"     -> "6x11",     "E21"    -> "5x11",
    "G21"    -> "5x11",     "G106"   -> "18",      "G49"    -> "18",       "G60"     -> "5x11",     "E60"    -> "5x11",
    "G61"    -> "5x11",     "E61"    -> "5x11",    "E38"    -> "5x11",     "G38"     -> "5x11",     "G05"    -> "5x11",
    "E05"    -> "5x11",     "G06"    -> "5x11",    "E06"    -> "5x11",     "E42"     -> "5x11",     "G42"    -> "5x11",
    "G103"   -> "18",       "E76"    -> "5x11",    "G76"    -> "5x11",     "E56"     -> "5x11",     "G56"    -> "5x11",
    "E66"    -> "5x11",     "G66"    -> "5x11",    "E43"    -> "5x11",     "G43"     -> "5x11",     "E70"    -> "5x11",
    "G70"    -> "5x11",     "E01"    -> "5x11",    "G01"    -> "5x11",     "E39"     -> "10x20",    "G64"    -> "10x12",
    "E64"    -> "10x12",    "G84"    -> "10x12",   "E84"    -> "10x12",    "G09"     -> "10x16",    "E09"    -> "10x16",
    "G50"    -> "10x16",    "E50"    -> "10x16",   "G91"    -> "10x20",    "E91"     -> "10x20",    "G92"    -> "10x20",
    "E92"    -> "10x20",    "G67"    -> "8x20",    "E67"    -> "8x20",     "G100"    -> "10x20",    "E100"   -> "10x20",
    "G62"    -> "10x25",    "E62"    -> "10x25",   "G104"   -> "10x25",    "E104"    -> "10x25",    "G20"    -> "10x25",
    "E20"    -> "10x25",    "G72"    -> "10x25",   "E87"    -> "10x25",    "G95"     -> "10x25",    "E11"    -> "10x25",
    "G45"    -> "12.5x25",  "E112"   -> "10x20",   "G46"    -> "12.5x31",  "E88"     -> "16x25",    "G105"   -> "10x31",
    "E45"    -> "10x31",    "G39"    -> "10x20",   "E16"    -> "10x12",    "G16"     -> "10x12",    "E31"    -> "10x16",
    "G31"    -> "10x16",    "E15"    -> "10x16",   "G15"    -> "10x16",    "E63"     -> "10x16",    "G63"    -> "10x16",
    "E55"    -> "10x16",    "G55"    -> "10x16",   "E32"    -> "8x20",     "G32"     -> "8x20",     "E99"    -> "10x20",
    "G99"    -> "10x20",    "E07"    -> "10x20",   "G07"    -> "10x20",    "E41"     -> "10x20",    "G41"    -> "10x20",
    "E53"    -> "10x20",    "G53"    -> "10x20",   "E83"    -> "8x20",     "G83"     -> "8x20",     "E19"    -> "8x20",
    "G19"    -> "8x20",     "E18"    -> "12.5x20", "G18"    -> "12.5x20",  "E106"    -> "12.5x15",  "G97"    -> "12.5x15",
    "E111"   -> "12.5x20",  "G96"    -> "12.5x20", "E113"   -> "12.5x20",  "E98"     -> "12.5x20",  "E110"   -> "10x16",
    "G33"    -> "8x15",     "E33"    -> "8x15",    "G08"    -> "8x15",     "E08"     -> "8x15",     "G68"    -> "8x11",
    "E68"    -> "8x11",     "G29"    -> "8x11",    "E29"    -> "8x11",     "G51"     -> "8x11",     "E51"    -> "8x11",
    "G27"    -> "8x11",     "E27"    -> "8x11",    "G85"    -> "10x16",    "E85"     -> "10x16",    "G04"    -> "6x11",
    "E04"    -> "6x11",     "G57"    -> "6x11",    "E57"    -> "6x11",     "G40"     -> "6x11",     "E40"    -> "6x11",
    "G22"    -> "6x11",     "E22"    -> "6x11",    "G25"    -> "10x20",    "E25"     -> "10x20",    "G78"    -> "10x16",
    "E78"    -> "10x16",    "G82"    -> "8x11",    "E82"    -> "8x11",     "G69"     -> "4x7",      "E69"    -> "4x7",
    "G26"    -> "6.3x7",    "E26"    -> "6.3x7",   "G110"   -> "10x16",    "E36"     -> "8x15",     "G36"    -> "8x15",
    "E35"    -> "8x15",     "G35"    -> "8x15",    "E23"    -> "8x11",     "G23"     -> "8x11",     "E58"    -> "8x11",
    "G58"    -> "8x11",     "E65"    -> "8x11",    "G65"    -> "8x11",     "E59"     -> "8x11",     "G59"    -> "8x11",
    "E86"    -> "8x11",     "G86"    -> "8x11",    "E37"    -> "6x11",     "G37"     -> "6x11",     "E17"    -> "6x11",
    "G17"    -> "6x11",     "E03"    -> "10x16",   "G03"    -> "10x16",    "E12"     -> "6x11",     "G12"    -> "6x11",
    "E44"    -> "8x15",     "G44"    -> "8x15",    "E74"    -> "8x15",     "G74"     -> "8x15",     "E10"    -> "8x15",
    "G10"    -> "8x15",     "E54"    -> "10x16",   "G54"    -> "10x16",    "E28"     -> "8x15",     "G28"    -> "8x15",
    "A001"   -> "Dummy8x15", "A002" -> "Dummy10x16", "B001" -> "Dummy12.5x20"
  )

  def apply(line: String) = Try {
    val columns = line.split(" ");
    val machineID = columns(8)
    val timestamp = columns(4).toLong
    new Record(
      columns(0), 
      columns(1), 
      columns(2).toLong, 
      columns(3).toLong,
      columns(4).toLong,
      columns(5).toLong, 
      columns(6),
      columns(7).toLong,
      columns(8),
      columns(9),
      columns(10),
      columns(11),
      columns(12),
      columns(13),
      productMapping.get(machineID).getOrElse("Unknown"),
      dateFormatter.format(new Date(timestamp * 1000))
    )
  }
}
