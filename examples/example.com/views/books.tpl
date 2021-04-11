<table>
	<tbody>
	{{ #books }}
	<tr>
		<td>{{ .author }}</td>
		<td>{{ .title }}</td>
	</tr>	
	{{ /books }}
	</tbody>
</table>
